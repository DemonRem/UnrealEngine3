REM this will update the latestUnrealEngine3_Delta label with the label you pass in

REM the label is already created p4 label -t labelFoo labelBar

p4 -u build_machine tag -l latestUnrealEngine3_Delta @%1

REM p4 tag -l latestUnrealEngine3-msew @%1






