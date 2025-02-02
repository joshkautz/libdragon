ROMNAME=large-font-with-outline
ROMTITLE=large-font-with-outline

BUILD_DIR = build
ASSETS_DIR = assets
FILESYSTEM_DIR = filesystem

include $(N64_INST)/include/n64.mk

SRC = main.c

ASSETS_TTF = $(wildcard $(ASSETS_DIR)/*.ttf)
ASSETS_CONV = $(addprefix $(FILESYSTEM_DIR)/,$(notdir $(ASSETS_TTF:%.ttf=%.font64)))

MKFONT_FLAGS = --range 41-5A --range 30-39 --range 20-20 --range 2E-2E --output $(FILESYSTEM_DIR)

all: $(ROMNAME).z64

$(ROMNAME).z64: N64_ROM_TITLE=$(ROMTITLE)
$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs

$(BUILD_DIR)/$(ROMNAME).elf: $(SRC:%.c=$(BUILD_DIR)/%.o)
$(BUILD_DIR)/$(ROMNAME).dfs: $(ASSETS_CONV)

# Convert font files
$(FILESYSTEM_DIR)/%.font64: $(ASSETS_DIR)/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [TTF to FONT64] $(basename $@)-20$(suffix $@)"
	$(N64_MKFONT) $(MKFONT_FLAGS) --size 20 "$<"
	@mv "$@" "$(basename $@)-20$(suffix $@)"

	@echo "    [TTF to FONT64] $(basename $@)-30$(suffix $@)"
	$(N64_MKFONT) $(MKFONT_FLAGS) --size 30 "$<"
	@mv "$@" "$(basename $@)-30$(suffix $@)"

	@echo "    [TTF to FONT64] $(basename $@)-50$(suffix $@)"
	$(N64_MKFONT) $(MKFONT_FLAGS) --size 50 "$<"
	@mv "$@" "$(basename $@)-50$(suffix $@)"

	@echo "    [TTF to FONT64] $(basename $@)-20-outlined$(suffix $@)"
	$(N64_MKFONT) $(MKFONT_FLAGS) --size 20 --outline 1 "$<"
	@mv "$@" "$(basename $@)-20-outlined$(suffix $@)"

	@echo "    [TTF to FONT64] $(basename $@)-30-outlined$(suffix $@)"
	$(N64_MKFONT) $(MKFONT_FLAGS) --size 30 --outline 1 "$<"
	@mv "$@" "$(basename $@)-30-outlined$(suffix $@)"

	@echo "    [TTF to FONT64] $(basename $@)-50-outlined$(suffix $@)"
	$(N64_MKFONT) $(MKFONT_FLAGS) --size 50 --outline 1 "$<"
	@mv "$@" "$(basename $@)-50-outlined$(suffix $@)"

clean:
	rm -rf $(FILESYSTEM_DIR) $(BUILD_DIR) *.z64

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
