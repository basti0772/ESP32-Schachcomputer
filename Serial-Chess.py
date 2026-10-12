"""Serial command client and independently validated PC-versus-ESP32 test game.
Install test-only dependencies with: python -m pip install --target .test-deps python-chess pyserial
"""
import sys, time, pathlib, subprocess, argparse
ROOT = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / '.test-deps'))
import serial
import chess
import chess.pgn

parser = argparse.ArgumentParser()
parser.add_argument('commands', nargs='*')
parser.add_argument('--game', action='store_true')
args = parser.parse_args()
port = subprocess.check_output(['powershell', '-ExecutionPolicy', 'Bypass', '-File', str(ROOT/'Find-ESP32Port.ps1')], text=True).strip()
link = serial.Serial()
link.port, link.baudrate, link.timeout = port, 115200, 0.1
link.dtr = False
link.rts = False
link.open()
log = open(ROOT/'serial-chess.log', 'a', encoding='utf-8')
def say(s):
    print(s, flush=True)
    log.write(s+'\n'); log.flush()
def command(s):
    link.reset_input_buffer()
    say('> '+s)
    link.write((s+'\n').encode())
    lines=[]; deadline=time.monotonic()+60
    while time.monotonic()<deadline:
        line=link.readline().decode(errors='replace').strip()
        if not line: continue
        say(line); lines.append(line)
        if line=='READY': return lines
    raise TimeoutError(s)

def validate(lines,board):
    for line in lines:
        if line.startswith(('MOVE ','ENGINE ')):
            move=chess.Move.from_uci(line.split()[1])
            assert move in board.legal_moves, ('illegal', line, board.fen())
            board.push(move)
        if line.startswith('FEN '):
            assert line[4:]==board.fen(en_passant='fen'), (line,board.fen(en_passant='fen'))
        assert not line.startswith('ERROR'), line

VALUES={1:100,2:320,3:335,4:500,5:900,6:0}
def evaluate(board):
    score=0
    for sq,p in board.piece_map().items():
        f,r=chess.square_file(sq),chess.square_rank(sq)
        advancement=r if p.color else 7-r
        center=7-abs(2*f-7)-abs(2*r-7)
        bonus=advancement*10 if p.piece_type==1 else center*8 if p.piece_type in (2,3) else 0
        score+=(VALUES[p.piece_type]+bonus)*(1 if p.color==board.turn else -1)
    return score

def search(board,depth,alpha,beta,ply=0):
    moves=list(board.legal_moves)
    if not moves: return -30000+ply if board.is_check() else 0
    if board.is_repetition(2) or board.is_fifty_moves(): return 0
    if depth<=0: return evaluate(board)
    moves.sort(key=lambda m: (board.is_capture(m),m.promotion or 0),reverse=True)
    for m in moves:
        board.push(m); score=-search(board,depth-1,-beta,-alpha,ply+1); board.pop()
        if score>=beta: return beta
        alpha=max(alpha,score)
    return alpha

def choose(board):
    best=None; alpha=-32000
    for m in sorted(board.legal_moves,key=lambda m:(board.is_capture(m),m.promotion or 0),reverse=True):
        board.push(m); score=-search(board,2,-32000,-alpha); board.pop()
        if score>alpha: best,alpha=m,score
    return best
try:
    if not args.game:
        for s in args.commands or ['board']: command(s)
    else:
        board=chess.Board()
        validate(command('new'),board)
        result='*'
        for turn in range(160):
            lines=command('legal')
            actual=set(next(x[6:] for x in lines if x.startswith('LEGAL ')).split())
            assert actual=={m.uci() for m in board.legal_moves}, 'legal move mismatch'
            move=choose(board)
            lines=command(move.uci()); validate(lines,board)
            results=[x[7:] for x in lines if x.startswith('RESULT ')]
            result=results[-1]
            if result!='*':
                assert board.is_game_over(claim_draw=True), (result,board.fen())
                assert result.split()[0]==board.result(claim_draw=True)
                break
        else:
            command('resign'); result='0-1 Aufgabe nach Testlimit'
        game=chess.pgn.Game.from_board(board)
        game.headers.update(Event='Serial integration test',White='PC test opponent (Python depth 3)',Black='ESP32 Chess',Result=result.split()[0])
        (ROOT/'test-game.pgn').write_text(str(game)+'\n',encoding='utf-8')
        say('VERIFIED GAME '+result+' plies='+str(len(board.move_stack)))
finally:
    link.close(); log.close()
