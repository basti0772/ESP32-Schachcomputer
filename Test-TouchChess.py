import sys,time,pathlib,subprocess,json
ROOT=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'.test-deps'))
import serial,chess
port=subprocess.check_output(['powershell','-ExecutionPolicy','Bypass','-File',str(ROOT/'Find-ESP32Port.ps1')],text=True).strip()
s=serial.Serial(); s.port=port; s.baudrate=115200; s.timeout=.1; s.dtr=False; s.rts=False
log=[]
def cmd(text):
    s.reset_input_buffer(); s.write((text+'\n').encode()); lines=[]; limit=time.monotonic()+40
    while time.monotonic()<limit:
        line=s.readline().decode(errors='replace').strip()
        if line: lines.append(line)
        if line=='READY':
            log.append({'command':text,'reply':lines}); return lines
    raise TimeoutError((text,lines))
def fen(): return next(x[4:] for x in cmd('board') if x.startswith('FEN '))
def sendmove(board,move):
    lines=cmd(move)
    for line in lines:
        if line.startswith(('MOVE ','ENGINE ')):
            m=chess.Move.from_uci(line.split()[1]); assert m in board.legal_moves; board.push(m)
        elif line.startswith('FEN '): assert line[4:]==board.fen(en_passant='fen')
        assert not line.startswith('ERROR'),line
    return lines
try:
    s.open()
    initial=cmd('touchinfo'); print(initial,flush=True)
    was_calibrated=any('calibrated=1 ' in l for l in initial)
    cmd('calibrate cancel')
    assert 'SELFTEST PASS' in cmd('selftest')
    cmd('new'); cmd('auto off'); b=chess.Board()
    cmd('tap 131 185'); assert any('selected=20 ' in x for x in cmd('touchinfo'))
    cmd('tap 131 107'); assert fen()==b.fen(en_passant='fen')
    sendmove(b,'tap 131 133'); assert b.peek()==chess.Move.from_uci('e2e4')
    print('PASS touch selection, invalid target, e2e4',flush=True)
    for promotion,y in [('q',110),('r',144),('b',178),('n',212)]:
        cmd('new'); cmd('auto off'); b=chess.Board()
        for move in 'a2a4 h7h5 a4a5 h5h4 a5a6 h4h3 a6b7 h3g2'.split(): sendmove(b,move)
        cmd('tap 53 55'); cmd('tap 27 29')
        assert any('promotion=112' in l for l in cmd('touchinfo'))
        sendmove(b,f'tap 274 {y}')
        assert b.peek().uci()=='b7a8'+promotion
    print('PASS all four touch promotions',flush=True)
    cmd('new'); b=chess.Board(); cmd('tap 131 185')
    sendmove(b,'tap 131 133'); assert len(b.move_stack)==2
    print('PASS touch move with automatic engine reply',flush=True)
    before=fen(); cmd('tap 274 144'); assert fen()==before
    lines=cmd('tap 274 144'); b=chess.Board()
    for line in lines:
        if line.startswith('ENGINE '):
            m=chess.Move.from_uci(line.split()[1]); assert m in b.legal_moves; b.push(m)
    assert len(b.move_stack)==1 and fen()==b.fen(en_passant='fen')
    print('PASS confirmed new game as Black, engine starts',flush=True)
    cmd('calibrate'); assert any('calibrating=1 ' in x for x in cmd('touchinfo'))
    assert any(x.startswith('FEN ') for x in cmd('board'))
    cmd('calibrate cancel'); cmd('new')
    if not was_calibrated: cmd('calibrate')
    print('PASS terminal remains responsive during calibration',flush=True)
    print('ALL TOUCH SOFTWARE TESTS PASS; physical touch/calibration requires user',flush=True)
finally:
    s.close(); (ROOT/'touch-test-results.json').write_text(json.dumps(log,indent=2),encoding='utf-8')
