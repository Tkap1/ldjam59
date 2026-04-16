@echo off

@REM pushd src
@REM "C:\Users\34687\Desktop\Apps\ctags-p6.0.20230416.0-x64\ctags.exe" -R -f .tags --exclude=gen_meta --exclude=src/external --languages=C++ --langmap=C++:.cpp.h
@REM popd

@REM "C:\Users\PakT\Downloads\ctags-2022-12-05_p5.9.20221204.0-5-gf9d21e7-x64\ctags.exe" -R -f .tags --kinds-C=+l

"C:\Users\34687\Desktop\Apps\ctags-p6.0.20230416.0-x64\ctags.exe" -f .tags --languages=C++ --langmap=C++:.cpp.h src/*