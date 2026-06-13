import asyncio
import socket
import struct
import time
import random
from enum import Enum
from rich.console import Console
from rich.table import Table
from prompt_toolkit import PromptSession
from prompt_toolkit.patch_stdout import patch_stdout

console = Console()

# ==========================================
# 1. State Machine & Opcode Definitions
# ==========================================
class EBotState(Enum):
    DISCONNECTED = 0
    CONNECTING = 1
    AUTH_WAIT = 2
    ACTIVE = 3
    TERMINATED = 4

class Opcode(Enum):
    CS_AUTH_REQ = 1
    CS_MOVE = 2
    CS_ATTACK = 3
    CS_SCENARIO = 4

SERVER_IP = "127.0.0.1"
SERVER_PORT = 9000

# ==========================================
# 2. DummyBot Class (Individual Client)
# ==========================================
class DummyBot:
    def __init__(self, bot_id, manager):
        self.bot_id = bot_id
        self.manager = manager
        self.state = EBotState.DISCONNECTED
        self.sequence = 1
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # Prevent blocking issues in asyncio
        self.sock.setblocking(False)
        
    async def change_state(self, new_state):
        self.state = new_state
        
    def create_packet(self, opcode):
        # Design 문서 SPacketHeader 명세: Size(uint16), Opcode(uint16), Sequence(uint32)
        size = 8
        packet = struct.pack('<HHL', size, opcode, self.sequence)
        self.sequence += 1
        return packet

    async def send_packet(self, opcode):
        # 패킷 로스 (네트워크 장애 주입) 시뮬레이션
        if random.random() * 100 < self.manager.packet_loss_rate:
            self.manager.metrics['dropped'] += 1
            return 
            
        packet = self.create_packet(opcode)
        try:
            self.sock.sendto(packet, (SERVER_IP, SERVER_PORT))
            self.manager.metrics['sent'] += 1
        except BlockingIOError:
            pass
        except Exception as e:
            pass

    async def run(self):
        # FSM: 연결 중 -> 인증 대기 -> 활성 상태 전이
        await self.change_state(EBotState.CONNECTING)
        await asyncio.sleep(0.01) # 소켓 바인딩 시뮬레이션
        
        await self.change_state(EBotState.AUTH_WAIT)
        await self.send_packet(Opcode.CS_AUTH_REQ.value)
        await asyncio.sleep(0.05) # 인증 응답 대기 시뮬레이션
        
        await self.change_state(EBotState.ACTIVE)
        
        # 활성 상태 로직 처리 (시나리오 패킷 전송)
        while self.state == EBotState.ACTIVE and self.manager.is_running:
            opcode = Opcode.CS_MOVE.value
            if self.manager.current_scenario == "attack":
                opcode = Opcode.CS_ATTACK.value
                
            await self.send_packet(opcode)
            
            # 초당 10번 (100ms 간격) 전송
            await asyncio.sleep(0.1)

# ==========================================
# 3. BotManager Class (Global Control)
# ==========================================
class BotManager:
    def __init__(self):
        self.bots = []
        self.packet_loss_rate = 0.0
        self.current_scenario = "move"
        self.is_running = True
        self.metrics = {'sent': 0, 'dropped': 0}
        self.tasks = []

    async def start_bots(self, count, delay=0.01):
        console.print(f"[green]Starting {count} bots with {delay}s spawn delay...[/green]")
        start_id = len(self.bots) + 1
        for i in range(count):
            if not self.is_running:
                break
            bot = DummyBot(start_id + i, self)
            self.bots.append(bot)
            task = asyncio.create_task(bot.run())
            self.tasks.append(task)
            
            if delay > 0:
                await asyncio.sleep(delay)
                
        console.print(f"[bold green]Successfully spawned {count} bots. Total bots: {len(self.bots)}[/bold green]")

    def set_packet_loss(self, rate):
        self.packet_loss_rate = float(rate)
        console.print(f"[yellow]Network Simulation: Packet loss set to {self.packet_loss_rate}%[/yellow]")

    def run_scenario(self, name):
        self.current_scenario = name
        console.print(f"[blue]Scenario changed to '{name}' for all active bots.[/blue]")

    def status(self):
        active_bots = sum(1 for b in self.bots if b.state == EBotState.ACTIVE)
        connecting_bots = sum(1 for b in self.bots if b.state in [EBotState.CONNECTING, EBotState.AUTH_WAIT])
        
        table = Table(title="Element-Pulse Dummy Manager Status")
        table.add_column("Metric", justify="left", style="cyan")
        table.add_column("Value", justify="right", style="magenta")
        
        table.add_row("Total Bots Spawned", str(len(self.bots)))
        table.add_row("Active (In-World) Bots", str(active_bots))
        table.add_row("Connecting/Auth Wait", str(connecting_bots))
        table.add_row("Configured Packet Loss", f"{self.packet_loss_rate}%")
        table.add_row("Current Bot Scenario", self.current_scenario)
        table.add_row("Total Packets Sent", str(self.metrics['sent']))
        table.add_row("Total Packets Dropped", str(self.metrics['dropped']))
        
        console.print(table)

    async def stop_all(self):
        self.is_running = False
        console.print("[yellow]Stopping all bots...[/yellow]")
        for bot in self.bots:
            await bot.change_state(EBotState.TERMINATED)
        console.print("[red]All bots gracefully terminated.[/red]")

# ==========================================
# 4. Interactive CLI Shell
# ==========================================
async def interactive_shell(manager):
    session = PromptSession("Element-Pulse Dummy Manager > ")
    while True:
        with patch_stdout():
            try:
                cmd_line = await session.prompt_async()
            except (EOFError, KeyboardInterrupt):
                break
                
        if not cmd_line.strip():
            continue
            
        parts = cmd_line.strip().split()
        cmd = parts[0].lower()
        args = parts[1:]
        
        try:
            if cmd == "start_bots":
                count = int(args[0]) if len(args) > 0 else 100
                delay = float(args[1]) if len(args) > 1 else 0.01
                asyncio.create_task(manager.start_bots(count, delay))
            elif cmd == "set_packet_loss":
                rate = float(args[0]) if len(args) > 0 else 0.0
                manager.set_packet_loss(rate)
            elif cmd == "run_scenario":
                name = args[0] if len(args) > 0 else "move"
                manager.run_scenario(name)
            elif cmd == "status":
                manager.status()
            elif cmd in ["exit", "quit", "stop_bots"]:
                await manager.stop_all()
                break
            elif cmd == "help":
                console.print("[bold]Available Commands:[/bold]")
                console.print("  [cyan]start_bots [count] [delay][/cyan] - Spawn N bots (default 100, 0.01s)")
                console.print("  [cyan]set_packet_loss [rate][/cyan]     - Set packet drop percentage (0-100)")
                console.print("  [cyan]run_scenario [name][/cyan]        - Change bot behavior (e.g., move, attack)")
                console.print("  [cyan]status[/cyan]                     - Show current system metrics")
                console.print("  [cyan]help[/cyan]                       - Show this help message")
                console.print("  [cyan]exit/quit/stop_bots[/cyan]        - Stop all bots and exit")
            else:
                console.print("[red]Unknown command. Type 'help' for available commands.[/red]")
        except ValueError:
            console.print("[red]Invalid argument type. Expected numbers for count/delay/rate.[/red]")

async def main():
    console.print("[bold cyan]==============================================[/bold cyan]")
    console.print("[bold cyan]===   Element-Pulse Dummy Manager CLI      ===[/bold cyan]")
    console.print("[bold cyan]==============================================[/bold cyan]")
    console.print("Type 'help' to see available commands.\n")
    
    manager = BotManager()
    await interactive_shell(manager)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
