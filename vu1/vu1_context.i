tLightPtrs_dir      .equ 0
tLightPtrs_point    .equ 1
tLightPtrs_spot     .equ 2
tLightPtrs_dummy    .equ 3

kNumLights          .equ kContextStart
kBackFaceCullMult   .equ kNumLights

kLightPointers0     .equ (kNumLights + 1)
kLightPointers1     .equ (kLightPointers0 + 1)
kLightPointers2     .equ (kLightPointers1 + 1)
kLightPointers3     .equ (kLightPointers2 + 1)
kLightPointers4     .equ (kLightPointers3 + 1)
kLightPointers5     .equ (kLightPointers4 + 1)
kLightPointers6     .equ (kLightPointers5 + 1)
kLightPointers7     .equ (kLightPointers6 + 1)

kLightAmbientOffset     .equ 0
kLightDiffuseOffset     .equ 1
kLightSpecularOffset    .equ 2
kLightPosOffset         .equ 3
kLightSpotDirOffset     .equ 4
kLightAttenCoeffOffset  .equ 5

kLightStructSize        .equ 6

kLight0Base         .equ (kLightPointers7 + 1)
kLight1Base         .equ (kLight0Base + kLightStructSize)
kLight2Base         .equ (kLight1Base + kLightStructSize)
kLight3Base         .equ (kLight2Base + kLightStructSize)
kLight4Base         .equ (kLight3Base + kLightStructSize)
kLight5Base         .equ (kLight4Base + kLightStructSize)
kLight6Base         .equ (kLight5Base + kLightStructSize)
kLight7Base         .equ (kLight6Base + kLightStructSize)

kGlobalAmbient      .equ (kLight7Base + kLightStructSize)

kClipToGsDepthOffset    .equ kGlobalAmbient

kMaterialEmission   .equ (kGlobalAmbient + 1)
kMaterialAmbient    .equ (kMaterialEmission + 1)
kMaterialDiffuse    .equ (kMaterialAmbient + 1)
kMaterialSpecular   .equ (kMaterialDiffuse + 1)

kVertexXfrm         .equ (kMaterialSpecular + 1)
kFixedVertToEye     .equ (kVertexXfrm + 4)
kObjToWorldXfrmTrans .equ (kFixedVertToEye + 1)
kWorldToObjXfrm     .equ (kObjToWorldXfrmTrans + 4)

kGifTag             .equ (kWorldToObjXfrm + 4)

kClipInfo           .equ (kGifTag + 1)

kContextLength      .equ (kClipInfo - kContextStart + 1)
