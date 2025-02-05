#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_WEEKS 60
#define MAX_LINE_LENGTH 1024
#define N_COINS 8

// Varing parameters
#define LEARNING_RATE 0.01
#define MAX_ITERATIONS 1000
#define TOLERANCE 1e-6

typedef struct {
    double close[N_COINS]; // Closing price
} WeeklyDataPoint;

int load_data_from_csv(char filenames[N_COINS][512], WeeklyDataPoint *data) {
    int current_week = 0;

    for (int i = 0; i < N_COINS; i++) {
        const char *filename = filenames[i];

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
};

double portfolio_variance(double weights[N_COINS], double cov_matrix[N_COINS][N_COINS]) {
    double variance = 0.0;
    for (int i = 0; i < N_COINS; i++) {
        for (int j = 0; j < N_COINS; j++) {
            variance += weights[i] * weights[j] * cov_matrix[i][j];
        }
    }
    return variance;
}

void gradient_descent(double target_return, double expected_returns[N_COINS], double cov_matrix[N_COINS][N_COINS], double weights[N_COINS]) {
    // Initialize weights randomly
    for (int i = 0; i < N_COINS; i++) {
        weights[i] = 1.0 / N_COINS; // Equal weights as initial guess
    }

    for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
        // Calculate gradient of portfolio variance
        double gradient[N_COINS];
        for (int i = 0; i < N_COINS; i++) {
            gradient[i] = 0.0;
            for (int j = 0; j < N_COINS; j++) {
                gradient[i] += 2 * weights[j] * cov_matrix[i][j];
            }
        }

        // Update weights
        for (int i = 0; i < N_COINS; i++) {
            weights[i] -= LEARNING_RATE * gradient[i];
        }

        // Project weights onto the simplex
        double sum = 0.0;
        for (int i = 0; i < N_COINS; i++) {
            if (weights[i] < 0) weights[i] = 0; // Ensure non-negative weights
            sum += weights[i];
        }
        for (int i = 0; i < N_COINS; i++) {
            weights[i] /= sum; // Normalize to sum to 1
        }

        // Check for convergence
        double current_return = 0.0;
        for (int i = 0; i < N_COINS; i++) {
            current_return += weights[i] * expected_returns[i];
        }
        if (fabs(current_return - target_return) < TOLERANCE) {
            break;
        }
    }
}


int main() {
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
        // Weekly returns
        for (int i = 0; i < N_COINS; i++)
        {
            for (int j = 0; j < MAX_WEEKS; j++) {
                if (i < MAX_WEEKS - 1){
                    weakly_returns[j][i] = (weekly_data[j + 1].close[i] - weekly_data[j].close[i]) / weekly_data[j].close[i];
                }
         
            }
        }

        //Markowitz
        double expected_returns[N_COINS];
        double cov_matrix[N_COINS][N_COINS];
        calculate_expected_returns(weakly_returns, expected_returns);
        calculate_cov_matrix(weakly_returns, expected_returns, cov_matrix);        

        // Defining target return (From were the efficient frontier will be calculated)
        double target_return = 0.0;
        for (int i = 0; i < N_COINS; i++) {
            target_return += expected_returns[i];
        }
        target_return /= N_COINS;

        // Optimize with gradient descent Method
        double weights[N_COINS];
        gradient_descent(target_return, expected_returns, cov_matrix, weights);


        printf("\nOptimized Portfolio Weights:\n");
        for (int i = 0; i < N_COINS; i++) {
            printf("%s: %.4f\n", coin_names[i], weights[i]);
        }

        // Portfolio variance and return
        double portfolio_var = portfolio_variance(weights, cov_matrix);
        double portfolio_return = 0.0;
        for (int i = 0; i < N_COINS; i++) {
            portfolio_return += weights[i] * expected_returns[i];
        }
        printf("\nPortfolio Return: %.4f\n", portfolio_return);
        printf("Portfolio Variance: %.4f\n", portfolio_var);

    } else {
        printf("Error loading data.\n");
        return 1;
    }
    return 0;
}