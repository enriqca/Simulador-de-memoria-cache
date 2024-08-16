#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>

typedef struct {
    uint32_t tag;
    int valido;
}Linha;

typedef struct {
    Linha *linhas;
    int tamanho;
    int index;
} Grupo;

typedef struct {
    Grupo *grupos;
    int num_grupos;
    int linhas_por_grupo;
} MemoriaCache;

void iniciar_memoria(MemoriaCache *cache, int tamanho_total, int tamanho_linha, int linhas_por_grupo) {
    int numero_linhas = tamanho_total / tamanho_linha;
    cache->num_grupos = numero_linhas / linhas_por_grupo;
    cache->linhas_por_grupo = linhas_por_grupo;
    cache->grupos = (Grupo *)malloc(cache->num_grupos * sizeof(Grupo));

    for (int i = 0; i < cache->num_grupos; i++) {
        cache->grupos[i].tamanho = linhas_por_grupo;
        cache->grupos[i].index = 0;
        cache->grupos[i].linhas = (Linha *)malloc(linhas_por_grupo * sizeof(Linha));
        for (int j = 0; j < linhas_por_grupo; j++) {
            cache->grupos[i].linhas[j].valido = 0;
        }
    }
}

void free_cache(MemoriaCache *cache) {
    for (int i = 0; i < cache->num_grupos; i++) {
        free(cache->grupos[i].linhas);
    }
    free(cache->grupos);
}

uint32_t get_tag(uint32_t endereco, int offset_bits, int index_bits) {
    return endereco >> (offset_bits + index_bits);
}

int simulate_cache(MemoriaCache *cache, uint32_t endereco, int tamanho_linha, int index_bits) {
    int offset_bits = __builtin_ctz(tamanho_linha);  // Calculate offset bits based on line size
    int set_index = (endereco >> offset_bits) & ((1 << index_bits) - 1);
    uint32_t tag = get_tag(endereco, offset_bits, index_bits);
    Grupo *grupo = &cache->grupos[set_index];

    for (int i = 0; i < cache->linhas_por_grupo; i++) {
        if (grupo->linhas[i].valido && grupo->linhas[i].tag == tag) {
            return 1; // Hit
        }
    }

    // Miss - Use FIFO replacement policy
    Linha *line = &grupo->linhas[grupo->index];
    line->tag = tag;
    line->valido = 1;
    grupo->index = (grupo->index + 1) % cache->linhas_por_grupo;

    return 0; // Miss
}

void print_cache(MemoriaCache *cache, int tamanho_linha, FILE *saida) {
    fprintf(saida, "================\n");
    fprintf(saida, "IDX V ** ADDR **\n");
    for (int i = 0; i < cache->num_grupos * cache->linhas_por_grupo; i++) {
        int index_grupos = i / cache->linhas_por_grupo;
        int index_linhas = i % cache->linhas_por_grupo;
        Linha *linha = &cache->grupos[index_grupos].linhas[index_linhas];

        fprintf(saida, "%03d %d ", i, linha->valido);
        if (linha->valido) fprintf(saida, "0x%08X\n", linha->tag);
        else fprintf(saida, "\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Uso: %s <tamanho_cache> <tamanho_linha> <linhas_por_grupo> <arquivo_acessos>\n", argv[0]);
        return 1;
    }

    int tamanho_cache = atoi(argv[1]);
    int tamanho_linha = atoi(argv[2]);
    int linhas_por_grupo = atoi(argv[3]);
    char *arq_entrada = argv[4];

    MemoriaCache cache;
    iniciar_memoria(&cache, tamanho_cache, tamanho_linha, linhas_por_grupo);

    FILE *entrada = fopen(arq_entrada, "r");
    if (!entrada) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    FILE *saida = fopen("output.txt", "w");
    if (!saida) {
        perror("Erro ao abrir arquivo de saída");
        fclose(entrada);
        return 1;
    }

    uint32_t endereco;
    int hits = 0, misses = 0;
    int index_bits = __builtin_ctz(cache.num_grupos);  // Calculate index bits

    while (fscanf(entrada, "%x", &endereco) != EOF) {
        if (simulate_cache(&cache, endereco, tamanho_linha, index_bits)) {
            hits++;
        } else {
            misses++;
        }
        print_cache(&cache, tamanho_linha, saida);
    }

    fprintf(saida, "#hits: %d\n", hits);
    fprintf(saida, "#miss: %d\n", misses);

    fclose(entrada);
    fclose(saida);
    free_cache(&cache);

    return 0;
}