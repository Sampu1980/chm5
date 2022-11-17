cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

set(coverity_ignore "!file('/googletest/')")
ADD_CUSTOM_TARGET(coverity
  COMMAND /opt/Coverity/bin/cov-build --dir coverity_results/ make VERBOSE=1
  COMMAND /opt/Coverity/bin/cov-analyze --dir coverity_results/ --all --tu-pattern \"${coverity_ignore}\"
  COMMAND /opt/Coverity/bin/cov-format-errors --dir coverity_results/ --html-output coverity_results/html
  COMMENT "Make Coverity Build"
)

