import subprocess
import sys

def parse_func(lines, line_index):
    line = lines[line_index]
    assert('<' in line and '>' in line)
    data = line.split()
    func_addr = int(data[0], 16)
    func_name = data[1].split('<')[1].split('>')[0]
    line_index += 1

    function = {}
    function['name'] = func_name
    function['address'] = func_addr

    instructions = []

    while len(lines[line_index]):
        line = lines[line_index]
        data = line.split('\t')
        addr = None
        opcode = None
        instr = None
        comment = None
        if len(data) >= 1:
            addr = int(data[0].strip().split(':')[0], 16)

        if len(data) >= 2:
            opcode = data[1].strip()

        if len(data) >= 3:
            commented_instr = data[2].strip().split('#')
            if len(commented_instr) > 1:
                instr = commented_instr[0].strip()
                comment = commented_instr[1].strip()
            else:
                instr = commented_instr[0].strip()
                comment = ""

        instructions.append({
            'addr' : addr,
            'opcode' : opcode,
            'instr' : instr,
            'comment' : comment,
        })

        line_index += 1

    function['instructions'] = instructions

    return function, line_index

def print_function(func):
    print('{} @ {}'.format(func['name'], func['address']))
    for instr in func['instructions']:
        print('  {}'.format(instr['instr']))
    print()


def parse_text_dump(txt):
    lines = txt.split('\n')

    result = []

    line_index = 0
    while 'Disassembly of section' not in lines[line_index]:
        line_index += 1
    line_index += 1

    while line_index < len(lines):

        while not (line_index < len(lines) and len(lines[line_index]) > 0 and lines[line_index][0].isdigit()):
            line_index += 1
            if line_index >= len(lines):
                break
        if line_index >= len(lines):
            break

        func, line_index = parse_func(lines, line_index)

        result.append(func)

    return result


def main():
    if len(sys.argv) != 2:
        return
    result = subprocess.run(['objdump', '-dC', '-Mintel', '-j.text', sys.argv[1]], stdout=subprocess.PIPE)
    objdump_result = result.stdout.decode('utf-8')
    parse_result = parse_text_dump(objdump_result)

    for func in parse_result:
        print_function(func)


if __name__ == '__main__':
    main()