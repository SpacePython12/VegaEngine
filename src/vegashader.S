.section .data

VegaFragShaderData:
    .incbin "vegafrag.fs"
    .byte 0

.balign 2
.global VegaFragShaderSize, VegaFragShader
VegaFragShaderSize: .quad VegaFragShaderSize-VegaFragShaderData
VegaFragShader: .quad VegaFragShaderData

VegaVertShaderData:
    .incbin "vegavert.vs"
    .byte 0
.balign 2
.global VegaVertShaderSize, VegaVertShader
VegaVertShaderSize: .quad VegaVertShaderSize-VegaVertShaderData
VegaVertShader: .quad VegaVertShaderData