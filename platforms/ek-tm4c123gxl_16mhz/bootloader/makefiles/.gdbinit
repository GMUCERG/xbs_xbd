target remote | openocd 
file TM4C123GH6PM_BL.axf

define hook-step
monitor reg primask 1
end
define hookpost-step
monitor reg primask 0
end


