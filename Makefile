# =========================
# COMPILATORI
# =========================
CC   = mpicc
NVCC = nvcc

# =========================
# DIRECTORY
# =========================
CUDA_DIR  = cuda
UTILS_DIR = utils
BUILD_DIR = build

# =========================
# CUDA
# =========================
CUDA_HOME ?= /usr/local/cuda

# =========================
# FLAGS
# =========================
CFLAGS = -O3 -Wall -fopenmp \
         -I$(UTILS_DIR) -I$(CUDA_DIR) \
         -I$(CUDA_HOME)/include

NVCCFLAGS = -O3 \
            -gencode arch=compute_75,code=sm_75 \
            -gencode arch=compute_89,code=sm_89 \
            -I$(UTILS_DIR) -I$(CUDA_DIR) \
            -I$(CUDA_HOME)/include \
            -allow-unsupported-compiler

LDFLAGS = -L$(CUDA_HOME)/lib64 -lcudart -lm -lstdc++

# =========================
# SORGENTI
# =========================
SRC_MAIN = main.c \
                $(UTILS_DIR)/operations.c \
                $(UTILS_DIR)/performance.c

SRC_CU = $(CUDA_DIR)/gpuMult.cu

# =========================
# OGGETTI
# =========================
OBJ_MAIN = $(BUILD_DIR)/main.o \
                $(BUILD_DIR)/operations.o \
                $(BUILD_DIR)/performance.o \
                $(BUILD_DIR)/gpuMult.o

# =========================
# TARGET
# =========================
EXEC = $(BUILD_DIR)/parallelo

# =========================
# DEFAULT
# =========================
all: $(BUILD_DIR) $(EXEC)

# =========================
# BUILD DIR
# =========================
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# =========================
# PARALLELO
# =========================
$(EXEC): $(OBJ_MAIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# =========================
# C OBJECTS
# =========================
$(BUILD_DIR)/main.o:main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(UTILS_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# =========================
# CUDA OBJECTS
# =========================
$(BUILD_DIR)/gpuMult.o: $(CUDA_DIR)/gpuMult.cu | $(BUILD_DIR)
	$(NVCC) $(NVCCFLAGS) -c $< -o $@

# =========================
# RUN
# =========================
run_parallelo:
	mpirun -np $(shell echo $$(( $(P_R) * $(P_C) ))) ./$(EXEC) $(ARGS)

# =========================
# CLEAN
# =========================
clean:
	rm -rf $(BUILD_DIR)
