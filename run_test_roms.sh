#!/bin/bash

echo "$(date)" > serial.txt

./gbemu ../gb-test-roms/cpu_instrs/individual/01-special.gb > /dev/null
./gbemu ../gb-test-roms/cpu_instrs/individual/02-interrupts.gb > /dev/null
./gbemu ../gb-test-roms/cpu_instrs/individual/03-op\ sp\,hl.gb > /dev/null
./gbemu ../gb-test-roms/cpu_instrs/individual/04-op\ r\,imm.gb > /dev/null
./gbemu ../gb-test-roms/cpu_instrs/individual/05-op\ rp.gb > /dev/null
./gbemu ../gb-test-roms/cpu_instrs/individual/06-ld\ r\,r.gb > /dev/null
./gbemu ../gb-test-roms/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb > /dev/null
./gbemu ../gb-test-roms/cpu_instrs/individual/08-misc\ instrs.gb > /dev/null
./gbemu ../gb-test-roms/instr_timing/instr_timing.gb > /dev/null


colorize() {
    esc=$(printf "\033[")
    red="31m"
    green="32m"
    normal="0m"
    sed '/^$/d' "$@" | sed -e "s/Passed/$esc$green&$esc$normal\n/g" -e "s/^.*Failed.*$/$esc$red&$esc$normal\n/g"
}

cat serial.txt | colorize