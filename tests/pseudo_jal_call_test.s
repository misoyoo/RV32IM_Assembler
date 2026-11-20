.text

_start:
    call call_target
    jal jal_target
    tail tail_target
    nop

call_target:
    ret

jal_target:
    ret

tail_target:
    ret