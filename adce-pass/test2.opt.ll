; ModuleID = 'test2.ll'
source_filename = "test2.ll"

define dso_local i32 @foo(i32 %x, i32 %y) {
entry:
  %rx = alloca i32, align 4
  store i32 %x, ptr %rx, align 4
  %ry = alloca i32, align 4
  store i32 %y, ptr %ry, align 4
  %r0 = load i32, ptr %rx, align 4
  br label %COND

COND:                                             ; preds = %BODY, %entry
  %r2 = phi i32 [ %r0, %entry ], [ %r5, %BODY ]
  %r4 = icmp sgt i32 %r2, 1
  br i1 %r4, label %BODY, label %END

BODY:                                             ; preds = %COND
  %r5 = sub nsw i32 %r2, 1
  br label %COND

END:                                              ; preds = %COND
  ret i32 %r2
}
