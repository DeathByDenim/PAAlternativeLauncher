PATH=%PATH%;"C:\Program Files (x86)\WiX Toolset v3.11\bin"
candle installer.wxs -ext WixUiExtension -ext WixUtilExtension
light installer.wixobj -ext WixUiExtension -ext WixUtilExtension
