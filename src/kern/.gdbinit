set auto-load local-gdbinit on

set confirm off
set architecture riscv:rv64
target remote tcp::26000
set disassemble-next-line auto
set riscv use-compressed-breakpoints yes
define dis_rv
    set $pc = $pc - 4
    disassemble $pc
end
document dis_rv
    Disassemble the instruction at the current program counter.
end
define si_rv
    set $pc = $pc - 4
    stepi
    dis_rv
end
document si_rv
    Step one instruction and disassemble it.
end
define ni_rv
    set $pc = $pc - 4
    nexti
    dis_rv
end
document ni_rv
    Step one instruction and disassemble it.
end
define si_n_rv
    set $pc = $pc - 4
    stepi $arg0
    dis_rv
end
document si_n_rv
    Step N instructions and disassemble the last one.
end
define reconnect_tcp
    target remote tcp::26000
end

add-symbol-file ../user/bin/init2 0x00000000C0000000
