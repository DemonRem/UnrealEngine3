@echo off
examplegame make -updateinisauto %1 %2 %3 %4 %5 %6
@if errorlevel 1 (
	set game=ExampleGame
	goto builderror
)

utgame make -updateinisauto %1 %2 %3 %4 %5 %6
@if errorlevel 1 (
	set game=UTGame
	goto builderror
)

geargame make -updateinisauto %1 %2 %3 %4 %5 %6
@if errorlevel 1 (
	set game=GearGame
	goto builderror
)

goto end

:builderror
@echo Script compilation error encountered while building %game%.

:end
