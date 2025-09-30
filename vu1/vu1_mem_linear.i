kContextStart       .equ 0

.include "vu1_context.i"

kDoubleBufBase      .equ (kContextStart + kContextLength)
kDoubleBufOffset    .equ ((1024 - kDoubleBufBase) / 2)
kDoubleBufSize      .equ kDoubleBufOffset

kNumVertices        .equ 0

kStripADCs          .equ (kNumVertices + 1)

kInputStart         .equ (kStripADCs + 4)

kInputBufSize       .equ (kDoubleBufSize / 2)
kOutputStart        .equ (0 + kInputBufSize)
kOutputBufSize      .equ (kDoubleBufSize - kOutputStart)
