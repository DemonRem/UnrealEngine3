@echo off

:start

p4 sync

call clean_all.bat

call build_all.bat

:end
