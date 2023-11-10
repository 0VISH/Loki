define void @__0(){
%_0 = add i64 0, 1
%_1 = alloca i64
br label %__1
__1:
%_2 = add i64 0, 5
%_4 = load i64, i64* %_1
%_3 = icmp  eq i64 %_4, %_2
br i1 %_3, label %__2, label %__3
br label %__2
__2:
%_5 = add i64 0, 4
%_7 = load i64, i64* %_1
%_6 = add i64 %_7, %_0
store i64 %_6, i64* %_1
br label %__1
br label %__3
__3:
ret void
}