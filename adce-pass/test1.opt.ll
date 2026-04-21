; ModuleID = 'test1.ll'
source_filename = "test1.ll"

define dso_local i32 @foo(i32 %x) {
entry:
  %rx = alloca i32, align 4
  store i32 %x, ptr %rx, align 4
  %r1 = load i32, ptr %rx, align 4
  %r3 = icmp sgt i32 %r1, 12
  br i1 %r3, label %B1, label %B2

B1:                                               ; preds = %entry
  %r4 = load i32, ptr %rx, align 4
  %r5 = add nsw i32 %r4, 5
  br label %B3

B2:                                               ; preds = %entry
  %r7 = load i32, ptr %rx, align 4
  %r8 = add nsw i32 %r7, 6
  br label %B3

B3:                                               ; preds = %B2, %B1
  %r9 = phi i32 [ %r5, %B1 ], [ %r8, %B2 ]
  ret i32 %r9
}
