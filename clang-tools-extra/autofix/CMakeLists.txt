set(LLVM_LINK_COMPONENTS support)

add_clang_executable(auto-fix
  AutoFix.cpp
  )
target_link_libraries(auto-fix
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangSerialization
  clangTooling
  )
