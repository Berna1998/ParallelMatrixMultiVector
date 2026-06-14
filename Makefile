# =========================
# COMPILATORI
# =========================
CC   = mpicc
NVCC = nvcc

# =========================
# DIRECTORY
# =========================
MAIN_DIR  = main
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
         -I$(UTILS_DIR) -I$(MAIN_DIR) \
         -I$(CUDA_HOME)/include

NVCCFLAGS = -O3 \
            -gencode arch=compute_75,code=sm_75 \
            -gencode arch=compute_89,code=sm_89 \
            -I$(UTILS_DIR) -I$(MAIN_DIR) \
            -I$(CUDA_HOME)/include \
            -allow-unsupported-compiler

LDFLAGS = -L$(CUDA_HOME)/lib64 -lcudart -lm -lstdc++

# =========================
# SORGENTI
# =========================
SRC_PARALLELO = $(MAIN_DIR)/ParalleloGPU.c \
                $(UTILS_DIR)/operations.c \
                $(UTILS_DIR)/performance.c

SRC_CU = $(MAIN_DIR)/gpuMult.cu

# =========================
# OGGETTI
# =========================
OBJ_PARALLELO = $(BUILD_DIR)/ParalleloGPU.o \
                $(BUILD_DIR)/operations.o \
                $(BUILD_DIR)/performance.o \
                $(BUILD_DIR)/gpuMult.o

# =========================
# TARGET
# =========================
EXEC_PARALLELO = $(BUILD_DIR)/parallelo

# =========================
# DEFAULT
# =========================
all: $(BUILD_DIR) $(EXEC_PARALLELO)

# =========================
# BUILD DIR
# =========================
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# =========================
# PARALLELO
# =========================
$(EXEC_PARALLELO): $(OBJ_PARALLELO)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# =========================
# C OBJECTS
# =========================
$(BUILD_DIR)/%.o: $(MAIN_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(UTILS_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# =========================
# CUDA OBJECTS
# =========================
$(BUILD_DIR)/%.o: $(MAIN_DIR)/%.cu | $(BUILD_DIR)
	$(NVCC) $(NVCCFLAGS) -c $< -o $@

# =========================
# RUN
# =========================
run_parallelo:
	mpirun -np $(shell echo $$(( $(P_R) * $(P_C) ))) ./$(EXEC_PARALLELO) $(ARGS)

# =========================
# CLEAN
# =========================
clean:
	rm -rf $(BUILD_DIR)
