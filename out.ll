define void @__main(){
%_00 = add i64 0, 1
%_01 = alloca i64
br label %__01
__01:
%_02 = add i64 0, 4
%_04 = load i64, i64* %_01
%_03 = icmp  eq i64 %_04, %_02
br i1 %_03, label %__02, label %__03
br label %__02
__02:
%_06 = load i64, i64* %_01
%_05 = add i64 %_06, %_00
store i64 %_05, i64* %_01
br label %__01
br label %__03
__03:
ret void
}

