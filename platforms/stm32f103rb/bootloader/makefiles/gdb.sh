arm-none-eabi-gdb XBD_STM32F1.elf --eval-command="target extended-remote localhost:4242" --eval-command="load" --eval-command="monitor reset" --eval-command="dprintf printf_gdb,\"xbd_print: %s\",print_ptr" 
