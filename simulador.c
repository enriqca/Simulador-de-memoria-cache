#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_LINES 1000

typedef struct {
    uint32_t tag;
    int valid;
} CacheLine;

typedef struct {
    CacheLine *lines;
    int size;
    int index;
} CacheSet;

typedef struct {
    CacheSet *sets;
    int num_sets;
    int lines_per_set;
} Cache;

void init_cache(Cache *cache, int total_size, int line_size, int lines_per_set) {
    int num_lines = total_size / line_size;
    cache->num_sets = num_lines / lines_per_set;
    cache->lines_per_set = lines_per_set;
    cache->sets = (CacheSet *)malloc(cache->num_sets * sizeof(CacheSet));

    for (int i = 0; i < cache->num_sets; i++) {
        cache->sets[i].size = lines_per_set;
        cache->sets[i].index = 0;
        cache->sets[i].lines = (CacheLine *)malloc(lines_per_set * sizeof(CacheLine));
        for (int j = 0; j < lines_per_set; j++) {
            cache->sets[i].lines[j].valid = 0;
        }
    }
}

void free_cache(Cache *cache) {
    for (int i = 0; i < cache->num_sets; i++) {
        free(cache->sets[i].lines);
    }
    free(cache->sets);
}

uint32_t get_tag(uint32_t address, int offset_bits, int index_bits) {
    return address >> (offset_bits + index_bits);
}

int simulate_cache(Cache *cache, uint32_t address, int line_size, int index_bits) {
    int offset_bits = __builtin_ctz(line_size);  // Calculate offset bits based on line size
    int set_index = (address >> offset_bits) & ((1 << index_bits) - 1);
    uint32_t tag = get_tag(address, offset_bits, index_bits);
    CacheSet *set = &cache->sets[set_index];

    for (int i = 0; i < cache->lines_per_set; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            return 1; // Hit
        }
    }

    // Miss - Use FIFO replacement policy
    CacheLine *line = &set->lines[set->index];
    line->tag = tag;
    line->valid = 1;
    set->index = (set->index + 1) % cache->lines_per_set;

    return 0; // Miss
}

void print_cache(Cache *cache, int line_size, FILE *output) {
    fprintf(output, "================\n");
    for (int i = 0; i < cache->num_sets * cache->lines_per_set; i++) {
        int set_index = i / cache->lines_per_set;
        int line_index = i % cache->lines_per_set;
        CacheLine *line = &cache->sets[set_index].lines[line_index];

        fprintf(output, "%03d %d ", i, line->valid);
        if (line->valid) {
            fprintf(output, "0x%08X\n", line->tag);
        } else {
            fprintf(output, "\n");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Uso: %s <tamanho_cache> <tamanho_linha> <linhas_por_grupo> <arquivo_acessos>\n", argv[0]);
        return 1;
    }

    int cache_size = atoi(argv[1]);
    int line_size = atoi(argv[2]);
    int lines_per_set = atoi(argv[3]);
    char *filename = argv[4];

    Cache cache;
    init_cache(&cache, cache_size, line_size, lines_per_set);

    FILE *input = fopen(filename, "r");
    if (!input) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    FILE *output = fopen("output.txt", "w");
    if (!output) {
        perror("Erro ao abrir arquivo de saída");
        fclose(input);
        return 1;
    }

    uint32_t address;
    int hits = 0, misses = 0;
    int index_bits = __builtin_ctz(cache.num_sets);  // Calculate index bits

    while (fscanf(input, "%x", &address) != EOF) {
        if (simulate_cache(&cache, address, line_size, index_bits)) {
            hits++;
        } else {
            misses++;
        }
        print_cache(&cache, line_size, output);
    }

    fprintf(output, "#hits: %d\n", hits);
    fprintf(output, "#miss: %d\n", misses);

    fclose(input);
    fclose(output);
    free_cache(&cache);

    return 0;
}