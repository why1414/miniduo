# 检索src目录查找cpp为后缀文件，用shell指令的find
# src 中多级目录的.cpp文件都会被找出
SRCS := $(shell find src -name "*.cpp")
TEST_SRCS := $(shell find test -name "*.cpp") 

# 将srcs的后缀为.cpp的替换为.o, 并让o文件放到objs目录中
SRC_OBJS := $(SRCS:.cpp=.o)
TEST_OBJS := $(TEST_SRCS:.cpp=.o)

# 将src/前缀替换为objs/前缀，让o文件防到objs目录中
SRC_OBJS := $(SRC_OBJS:src/%=objs/src/%)
TEST_BINS := $(TEST_SRCS:test/%.cpp=%)
TEST_OBJS := $(TEST_OBJS:test/%=objs/test/%)

# 定义objs下的o文件，依赖src下对应的cpp文件
objs/src/%.o : src/%.cpp
	@mkdir -p $(dir $@)
	g++ -c -g $< -o $@

objs/test/%.o : test/%.cpp
	@mkdir -p $(dir $@)
	g++ -c -g -I ./src $< -o $@


all :  $(TEST_BINS)


# 我们把pro放到workspace下面
$(TEST_BINS) : %:workspace/%
workspace/% : objs/test/%.o $(SRC_OBJS)
	g++ -g -pthread -Wall -o $@ $^


debug : 
# @echo objs is [$(objs)]
	@echo $(TEST_BINS)

run-%: %
	@cd workspace && ./$< && cd ..
	

clean : clean_all

clean_all : 
	rm -rf workspace/* objs/*

clean_tests :
	rm -rf workspace/* objs/test/*

.PHONY : $(TEST_BINS) run-% debug test all clean
# .PRECIOUS : $(SRC_OBJS) $(TEST_OBJS) # 临时解决 %.o 文件自动删除的问题
.SECONDARY:


