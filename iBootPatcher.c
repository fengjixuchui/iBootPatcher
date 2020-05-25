#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>

#define ISB                    "\xDF\x3F\x03\xD5"
#define RET                    "\xC0\x03\x5F\xD6"
#define NOP                    "\x1F\x20\x03\xD5"

#define IM4P_ENC               "\x49\x4D\x34\x50"

#define TLBI_ALLE3             "\x1F\x87\x0E\xD5"
#define TLBI_VMALLE1           "\x1F\x87\x08\xD5"

#define MSR_VBAR_EL3_X10       "\x0A\xC0\x1E\xD5"
#define MSR_VBAR_EL1_X10       "\x0A\xC0\x18\xD5"

#define MRS_X0_SCTLR_EL3       "\x00\x10\x3E\xD5"
#define MRS_X0_SCTLR_EL1       "\x00\x10\x38\xD5"

#define MSR_SCTLR_EL3_X0       "\x00\x10\x1E\xD5"
#define MSR_SCTLR_EL1_X0       "\x00\x10\x18\xD5"

#define MSR_SCR_EL3_X0         "\x00\x11\x1E\xD5"

#define MSR_MAIR_EL3_X0        "\x00\xA2\x1E\xD5"
#define MSR_MAIR_EL1_X0        "\x00\xA2\x18\xD5"

#define MSR_TCR_EL3_X0         "\x40\x20\x1E\xD5"
#define MSR_TCR_EL1_X0         "\x40\x20\x18\xD5"

#define MSR_TTBR0_EL3_X0       "\x00\x20\x1E\xD5"
#define MSR_TTBR0_EL1_X0       "\x00\x20\x18\xD5"

#define MRS_X30_ELR_EL3        "\x3E\x40\x3E\xD5"
#define MRS_X30_ELR_EL1        "\x3E\x40\x38\xD5"

#define MRS_X1_ESR_EL3         "\x01\x52\x3E\xD5"
#define MRS_X1_ESR_EL1         "\x01\x52\x38\xD5"

#define MRS_X1_FAR_EL3         "\x01\x60\x3E\xD5"
#define MRS_X1_FAR_EL1         "\x01\x60\x38\xD5"

#define MRS_X2_ESR_EL3         "\x02\x52\x3E\xD5"
#define MRS_X2_ESR_EL1         "\x02\x52\x38\xD5"

#define MRS_X2_SPSR_EL3        "\x02\x40\x3E\xD5"
#define MRS_X2_SPSR_EL1        "\x02\x40\x38\xD5"

#define MSR_ELR_EL3_X0         "\x20\x40\x1E\xD5"
#define MSR_ELR_EL1_X0         "\x20\x40\x18\xD5"

#define MSR_SPSR_EL3_X1        "\x01\x40\x1E\xD5"
#define MSR_SPSR_EL1_X1        "\x01\x40\x18\xD5"

#define MRS_X2_SCTLR_EL3       "\x02\x10\x3E\xD5"
#define MRS_X2_SCTLR_EL1       "\x02\x10\x38\xD5"

#define MSR_SCTLR_EL3_X1       "\x01\x10\x1E\xD5"
#define MSR_SCTLR_EL1_X1       "\x01\x10\x18\xD5"

#define MSR_ELR_EL2_XZR        "\x3F\x40\x1C\xD5"
#define MSR_ELR_EL3_XZR        "\x3F\x40\x1E\xD5"

#define MSR_SPSR_EL2_XZR       "\x1F\x40\x1C\xD5"
#define MSR_SPSR_EL3_XZR       "\x1F\x40\x1E\xD5"

#define MSR_SP_EL1_XZR         "\x1F\x41\x1C\xD5"
#define MSR_SP_EL2_XZR         "\x1F\x41\x1E\xD5"

#define ORR_X0_X0_0x800000     "\x00\x00\x69\xB2"
#define ORR_X0_X0_0x10000000   "\x00\x00\x60\xB2"

/* i highly advice to read a little bit from here */
/* https://developer.arm.com/docs/ddi0595/b/aarch64-system-registers */

#define tcr_patches ORR_X0_X0_0x10000000 ORR_X0_X0_0x800000 MSR_TCR_EL1_X0 ISB RET

uint32_t asm_arm64_instruction(uint64_t src, uint64_t dest) {
  return src > dest ? 0x18000000 - (src - dest) / 4 : 0x14000000 + (dest - src) / 4;
}

void *apply_generic_el3_patches(void *ibot, void *img, unsigned int length) {
  unsigned int i = 0, j = 0;

  const char *patches[22][2] = {
    { MSR_VBAR_EL3_X10, MSR_VBAR_EL1_X10 },
    { MRS_X0_SCTLR_EL3, MRS_X0_SCTLR_EL1 },
    { MSR_SCTLR_EL3_X0, MSR_SCTLR_EL1_X0 },
    { MSR_SCR_EL3_X0,   NOP },
    { MSR_MAIR_EL3_X0,  MSR_MAIR_EL1_X0 },
    { MSR_TTBR0_EL3_X0, MSR_TTBR0_EL1_X0 },
    { MRS_X30_ELR_EL3,  MRS_X30_ELR_EL1 },
    { MRS_X1_ESR_EL3,   MRS_X1_ESR_EL1 },
    { MRS_X1_FAR_EL3,   MRS_X1_FAR_EL1 },
    { MRS_X2_ESR_EL3,   MRS_X2_ESR_EL1 },
    { MRS_X2_SPSR_EL3,  MRS_X2_SPSR_EL1 },
    { MSR_ELR_EL3_X0,   MSR_ELR_EL1_X0 },
    { MSR_SPSR_EL3_X1,  MSR_SPSR_EL1_X1 },
    { MRS_X2_SCTLR_EL3, MRS_X2_SCTLR_EL1 },
    { TLBI_ALLE3,       TLBI_VMALLE1 },
    { MSR_SCTLR_EL3_X1, MSR_SCTLR_EL1_X1 },
    { MSR_ELR_EL2_XZR,  NOP },
    { MSR_ELR_EL3_XZR,  NOP },
    { MSR_SPSR_EL2_XZR, NOP },
    { MSR_SPSR_EL3_XZR, NOP },
    { MSR_SP_EL1_XZR,   NOP },
    { MSR_SP_EL2_XZR,   NOP },
  };

  for (i = 0; i < length; i += 0x4) {
    for (j = 0; j < 22; j++) {
      if (memcmp(&img[i], patches[j][0], 0x4) == 0) {

        memcpy(&ibot[i], patches[j][1], 0x4);

        printf("[%s]: generic EL3 patches = 0x%x\n", __func__, i);
      }
    }
  }

  return ibot;
}

void *apply_tcr_el3_patches(void *ibot, void *img, unsigned int length) {
  unsigned int i = 0;

  for (; i < length; i += 0x4) {

    uint32_t patch = asm_arm64_instruction(i, 0x1EC);

    if (memcmp(&img[i], MSR_TCR_EL3_X0, 0x4) == 0) {

      memcpy(&ibot[i], &patch, sizeof(uint32_t));

      memcpy(&ibot[0x1EC], tcr_patches, 0x14);

      printf("[%s]: TCR EL3 patches = 0x%x\n", __func__, i);
      
      return ibot;
    }
  }

  return NULL;
}

void usage(char *owo[]) {
  char *ibot = NULL;
  ibot = strrchr(owo[0], '/');
  printf("usage: %s [-i] <input> <output> [--el1]\n", (ibot ? ibot + 1 : owo[0]));
  printf("\t-i, --image\tiOS 7.x - 8.x - 9.x iBoot64 input.\n");
  printf("\t-e, --el1\tapply generics + TCR patches (run from EL3 to EL1).\n");
}

int main(int argc, char *argv[]) {
  struct stat st;
  
  void *img = NULL;
  void *ibot = NULL;

  unsigned int fd, rv;
  unsigned int owo = 0, el1 = 0, length = 0;

  if (argc < 4) {
    goto usagend;
  }

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--image")) {
      owo = 1;
      if (!strcmp(argv[i+3], "-e") || !strcmp(argv[i+3], "--el1")) {
        el1 = 1;

        printf("[%s]: starting..\n", __func__);

        break;
      } else {
        printf("warning: unrecognized argument: %s\n", argv[i+3]);
        goto usagend;
      }
    } else {
      printf("warning: unrecognized argument: %s\n", argv[i]);
      goto usagend;
    }
  }

  if (owo) {
    fd = open(argv[2], O_RDONLY);

    if (!fd) {
      printf("[%s]: can't open %s.\n", __func__, argv[2]);
      return -1;
    }

    rv = fstat(fd, &st);

    length = st.st_size;

    img = (void *)malloc(length);
    
    rv = read(fd, img, length);

    assert(rv == length);

    ibot = (void *)malloc(length);

    memcpy(ibot, img, length);

    close(fd);

    if (*(uint32_t *)img == 0x496d6733) {
      printf("[%s]: IMG3 files are not supported.\n", __func__);
      goto end;
    }

    if (memcmp(&img[0x7], IM4P_ENC, 0x4) == 0) {
      printf("[%s]: packed IM4P container detected, only raw images are supported.\n", __func__);
      goto end;
    }

    if (*(uint32_t *)img != 0xea00000e && *(uint32_t *)img != 0x90000000) {
      printf("[%s]: this is not a valid iBoot64 image.\n", __func__);
      goto end;
    }

    printf("[%s]: detected %s!\n", __func__, img + 0x280);

    if (el1) {
      printf("[%s]: applying generic EL3 iBoot patches..\n", __func__);
      
      apply_generic_el3_patches(ibot, img, length);

      printf("[%s]: patching iBoot to run in EL1..\n", __func__);

      if (!apply_tcr_el3_patches(ibot, img, length)) {
        printf("[%s]: unable to find MSR TCR_EL3, X0 instruction.\n", __func__);
        goto end;
      }
    }

    fd = open(argv[3], O_CREAT | O_RDWR, 00644);

    if (!fd) { 
      printf("[%s]: unable to open %s!\n", __func__, argv[3]);
      goto end; 
    }

    printf("[%s]: writing %s..\n", __func__, argv[3]);

    write(fd, ibot, length);

    free(img);
    free(ibot);

    close(fd);

    printf("[%s]: done!\n", __func__);

    return 0;
  }

  usagend:
  usage(argv);
  return -1;

  end:
  free(img);
  free(ibot);
  return -1;
}