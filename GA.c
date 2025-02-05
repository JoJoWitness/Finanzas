#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_WEEKS 60
#define MAX_LINE_LENGTH 1024
#define N_COINS 8

// Varying parameters
#define POP_SIZE 1000
#define MAX_GENERATIONS 30
#define MUTATION_RATE 0.01

typedef struct {
    double close[N_COINS]; // Closing price
} WeeklyDataPoint;

typedef struct {
    double weights[N_COINS]; // Portfolio weights
    double fitness;          // Fitness value
} Portfolio;

int load_data_from_csv(char filenames[N_COINS][512], WeeklyDataPoint *data) {
    int current_week = 0;

    for (int i = 0; i < N_COINS; i++) {
        const char *filename = filenames[i];
        // printf("Loading File: %s\n", filename);

        FILE *file = fopen(filename, "r");
        
        if (file == NULL) {
            perror("Error opening file");
            return 1; 
        }

        char line[MAX_LINE_LENGTH];    
        fgets(line, MAX_LINE_LENGTH, file); // Skip the header line
    
        int week = 0;
        while (fgets(line, MAX_LINE_LENGTH, file) != NULL && week < MAX_WEEKS) {
            char *token;
            int column = 0;
            
            token = strtok(line, ";"); // First csv column
            
            while (token != NULL) {
                if(column == 8) {
                    data[week].close[i] = atof(token);
                    break; 
                }
                token = strtok(NULL, ";");
                column++;
            }
            week++;
        }
        fclose(file);
    } 
    return 0;
}

void calculate_expected_returns(double weakly_returns[MAX_WEEKS - 1][N_COINS], double expected_returns[N_COINS]) {
    for (int i = 0; i < N_COINS; i++) {
        expected_returns[i] = 0.0;
        for (int j = 0; j < MAX_WEEKS - 1; j++) {
            expected_returns[i] += weakly_returns[j][i];
        }
        expected_returns[i] /= (MAX_WEEKS - 1);
    }
}

void calculate_cov_matrix(double weakly_returns[MAX_WEEKS - 1][N_COINS], double expected_returns[N_COINS], double cov_matrix[N_COINS][N_COINS]) {
    for (int i = 0; i < N_COINS; i++) {
        for (int j = 0; j < N_COINS; j++) {
            cov_matrix[i][j] = 0.0;
            for (int k = 0; k < MAX_WEEKS - 1; k++) {
                cov_matrix[i][j] += (weakly_returns[k][i] - expected_returns[i])*(weakly_returns[k][j] - expected_returns[j]); 
            }
            cov_matrix[i][j] /= (MAX_WEEKS - 1);
        }
    }
}

double calculate_fitness(double weights[N_COINS], double expected_returns[N_COINS], double cov_matrix[N_COINS][N_COINS]) {
    double E_Rp = 0.0; // Fitness
    double var_Rp = 0.0; // δ^2(Rp)

    // Expected return E(Rp)
    for (int i = 0; i < N_COINS; i++) {
        E_Rp += weights[i] * expected_returns[i];
    }

    // Variance δ^2(Rp)
    for (int i = 0; i < N_COINS; i++) {
        for (int j = 0; j < N_COINS; j++) {
            var_Rp += weights[i] * weights[j] * cov_matrix[i][j];
        }
    }

    // Fitness = E(R_P) / δ^2(R_P)
    return E_Rp / var_Rp;
}

void init_population(Portfolio population[POP_SIZE]) {
    for (int i = 0; i < POP_SIZE; i++) {
        double sum = 0.0;
        for (int j = 0; j < N_COINS; j++) {
            population[i].weights[j] = (double) rand() / (double) RAND_MAX;
            sum += population[i].weights[j];
        }

        // Normalize weights to sum to 1
        for (int j = 0; j < N_COINS; j++) {
            population[i].weights[j] /= sum;
        } 
    }
}

void selection(Portfolio population[POP_SIZE]) {
    Portfolio new_pop[POP_SIZE];
    for (int i = 0; i < POP_SIZE; i++) {
        int a = rand() % POP_SIZE;
        int b = rand() % POP_SIZE;
        if (population[a].fitness > population[b].fitness) {
            new_pop[i] = population[a];
        } else {
            new_pop[i] = population[b];
        }
    }

    // Copy new_pop to population
    for (int i = 0; i < POP_SIZE; i++) {
        population[i] = new_pop[i];
    }
}

// Single-point crossover
void crossover(Portfolio population[POP_SIZE]) {
    for (int i = 0; i < POP_SIZE; i += 2) {
        int crossover_point = rand() % N_COINS;
        for (int j = crossover_point; j < N_COINS; j++) {
            double temp = population[i].weights[j];
            population[i].weights[j] = population[i + 1].weights[j];
            population[i + 1].weights[j] = temp;
        }
    }
}

void mutation(Portfolio population[POP_SIZE]) {
    for (int i = 0; i < POP_SIZE; i++) {
        for (int j = 0; j < N_COINS; j++) {
            if ((double)rand() / RAND_MAX < MUTATION_RATE) {
                population[i].weights[j] = (double)rand() / (double)RAND_MAX;
            }
        }

        // Re-normalize weights
        double sum = 0.0;
        for (int j = 0; j < N_COINS; j++) {
            sum += population[i].weights[j];
        }
        for (int j = 0; j < N_COINS; j++) {
            population[i].weights[j] /= sum;
        }
    }
}

int main() {
    srand(time(NULL)); // Seed the random number generator

    const char *coin_names[] = {
        "Aptos", "Bitcoin", "BNB", "Cardano", "Ethereum", "Solana", "Sui", "XRP"
    };

    char filenames[N_COINS][512]; 
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return 1;
    }

    for (int i = 0; i < N_COINS; i++) {
        snprintf(filenames[i], sizeof(filenames[i]), "%s/Historical_Last_13Month/%s_weekly_historical_data_coinmarketcap.csv",
                 cwd, coin_names[i]);
    }

    WeeklyDataPoint weekly_data[MAX_WEEKS];
    double weakly_returns[MAX_WEEKS - 1][N_COINS];
    
    int result = load_data_from_csv(filenames, weekly_data);
    if (result == 0) {
        printf("Data loaded successfully!\n");

        // Calculate weekly returns
        for (int i = 0; i < N_COINS; i++) {
            for (int j = 0; j < MAX_WEEKS - 1; j++) {
                weakly_returns[j][i] = (weekly_data[j + 1].close[i] - weekly_data[j].close[i]) / weekly_data[j].close[i];
            }
        }

        // Calculate expected returns and covariance matrix
        double expected_returns[N_COINS];
        double cov_matrix[N_COINS][N_COINS];
        calculate_expected_returns(weakly_returns, expected_returns);
        calculate_cov_matrix(weakly_returns, expected_returns, cov_matrix);        

        // Initialize population
        Portfolio population[POP_SIZE];
        init_population(population);

        // Run GA
        for (int generation = 0; generation < MAX_GENERATIONS; generation++) {
            // Evaluate fitness
            for (int i = 0; i < POP_SIZE; i++) {
                population[i].fitness = calculate_fitness(population[i].weights, expected_returns, cov_matrix);
            }

            // Track progress
            double best_fitness = population[0].fitness;
            for (int i = 1; i < POP_SIZE; i++) {
                if (population[i].fitness > best_fitness) {
                    best_fitness = population[i].fitness;
                }
            }
            printf("Generation %d: Best Fitness = %.4f\n", generation, best_fitness);

            selection(population);
            crossover(population);
            mutation(population);
        }

        // Find the best portfolio
        int best_index = 0;
        double best_fitness = population[0].fitness;
        
        for (int i = 1; i < POP_SIZE; i++) {
            if (population[i].fitness > best_fitness) {
                best_fitness = population[i].fitness;
                best_index = i;
            }
        }

        printf("\nBest Portfolio:\n");
        for (int i = 0; i < N_COINS; i++) {
            printf("%s: %.4f\n", coin_names[i], population[best_index].weights[i]);
        };

        printf("\nBest Fitness: %.4f\n", best_fitness);
        
    } else {
        printf("Error loading data.\n");
        return 1;
    }
    return 0;
}