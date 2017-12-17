arm-tiva_c-eabi-gdb --eval-command="source .gdbinit" --eval-command="load" --eval-command="monitor reset" --eval-command="detach" --eval-command="q"

#arm-tiva_c-eabi-gdb --eval-command="target remote | openocd" --eval-command="file TM4C129ENCPDT_BL.axf" --eval-command="load" --eval-command="detach" --eval-command="q"
