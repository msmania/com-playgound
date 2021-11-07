@echo off
set KEYS=^
  hkcr\typelib\{b2137105-d2ea-422c-9e89-3b02645d2078}^
  hkcr\interface\{c981d429-dd12-4e4c-b23c-a53172fcaede}^
  hkcr\interface\{06c56a36-5f16-4580-9a48-a6b828681d4a}^
  hkcr\interface\{aac80615-1103-4539-a5b0-02b6440cd1cc}^
  hkcr\clsid\{16C324E8-4B82-4648-81A0-E76E3639005E}^
  hkcr\clsid\{766F63F7-E338-4CC4-99C3-19428426E912}^
  hkcr\clsid\{8C88319B-6BE3-4D7C-8101-93E50DAF96AE}^
  hkcr\clsid\{51A5A35B-9266-4D80-9CB6-FD345FF5A0CC}
for %%i in (%KEYS%) do (reg query %%i /s)
