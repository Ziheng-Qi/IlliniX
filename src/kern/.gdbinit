set auto-load local-gdbinit on
add-auto-load-safe-path /

set confirm off
set architecture riscv:rv64
target remote tcp::26000
set disassemble-next-line auto
set riscv use-compressed-breakpoints yes
define setup-tcp
    target remote tcp::26000
end
