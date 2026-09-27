#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <math.h>
#pragma comment(lib, "user32.lib")

#define FILAS     17
#define COLUMNAS  19
#define SOLIDO  -1
#define AIRE      0
#define AGUA    1
#define MAX_PARTICULAS 120
#define Dt (1.0f / 60.0f)
#define DIST_MIN 0.5f

void imprimir_frame(void);
void inicializar_casillas(void);
void incializar_particulas(void);
void marcar_casillas_con_agua(void);
void movimiento_particulas(void);
void separar_particulas(void);
void P2G(void);
void condiciones_de_frontera(void);
void presion(void);
void G2P(void);
void leer_teclado(void);

int leds[FILAS][COLUMNAS] = {
    /*        0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18 */
    /* 0 */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /* 1 */ {-1, -1, -1, -1, -1,  0, -1, -1, -1, -1, -1, -1, -1,  0, -1, -1, -1, -1, -1},
    /* 2 */ {-1, -1, -1, -1,  0,  0,  0, -1, -1, -1, -1, -1,  0,  0,  0, -1, -1, -1, -1},
    /* 3 */ {-1, -1, -1,  0,  0,  0,  0,  0, -1, -1, -1,  0,  0,  0,  0,  0, -1, -1, -1},
    /* 4 */ {-1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1},
    /* 5 */ {-1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1},
    /* 6 */ {-1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1},
    /* 7 */ {-1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1},
    /* 8 */ {-1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1},
    /* 9 */ {-1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1},
    /*10 */ {-1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1},
    /*11 */ {-1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1},
    /*12 */ {-1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1, -1},
    /*13 */ {-1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1, -1, -1},
    /*14 */ {-1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0, -1, -1, -1, -1, -1, -1, -1, -1},
    /*15 */ {-1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*16 */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
};

typedef struct {
    float x, y;     // Posicion
    float u, v;     // Velocidad x, y 
    float x_ant, y_ant;  // Posicion al inicio del frame

} Particula;

typedef struct {
    //u: cara izquierda     v: cara superior
    float u, v;       // Componentes X y Y de las velocidades de la cara
    float u_prev, v_prev;   // copia antes de la presión (FLIP)
    float peso_u, peso_v;   // pesos para P2G
    float solido;        //Solido=1, libre=0
    int tipo;   //Solido, aire o agua
    float densidad;
} Casilla;

Casilla   casillas[FILAS][COLUMNAS];
Particula particulas[MAX_PARTICULAS];
int       num_particulas = 0;
float gx=0;
float gy=10;

//________________________________________________________________________________________________________________________________________________________
int main(void){
    srand(1);
    inicializar_casillas();
    incializar_particulas();
    //Ciclo de trabajo principal
    system("cls");
    COORD inicio = {0, 0};
    while(1){
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), inicio);
        leer_teclado();
        movimiento_particulas();
        separar_particulas();
        marcar_casillas_con_agua();
        P2G();
        condiciones_de_frontera();
        presion();
        G2P();
        imprimir_frame();
        Sleep(100);
    }
    return 0;
}

/*Esta funcion imprime una vez la matriz leds que actua como pantalla
    inputs: none
    outputs: la pantalla con cada casilla
*/
void imprimir_frame (void){
    int i;
    int j;
    
    for(i=0;i<FILAS;i++){
        for(j=0;j<COLUMNAS;j++){
            if(leds[i][j]==1){
                printf("X ");
            } 
            else if(leds[i][j]==0){
                printf("O ");
            }
            else{
                printf("  ");
            }
        }
        printf("\n");
    }
}
/*
    Esta funcion genera los valores iniciales de cada casilla
    inputs: none
    outputs: la matriz casillas[i][j] inicializadas

*/
void inicializar_casillas(void){
    int i, j;
    for(i=0;i<FILAS;i++){
        for(j=0;j<COLUMNAS;j++){
            if(leds[i][j]==-1){
                casillas[i][j].solido=1;
            } 
            casillas[i][j].u=0;
            casillas[i][j].v=0;
            casillas[i][j].u_prev=0;
            casillas[i][j].v_prev=0;
            casillas[i][j].peso_u=0;
            casillas[i][j].peso_v=0;
            casillas[i][j].densidad=0;
        }
    }
}
/*
    Esta funcion genera los valores iniciales de cada particula
    y las genera en casillas vacias disponibles, hasta abajo de la pantalla
    inputs: none
    outputs: la matriz particulas[i] inicializada
*/
void incializar_particulas(void){
    int i, j;
    num_particulas=0;
    for(i=FILAS-1;i>0;i--){
        for(j=COLUMNAS-1;j>0;j--){
            if (casillas[i][j].solido==0){
                if (num_particulas >= MAX_PARTICULAS) return;
                particulas[num_particulas].u=0;
                particulas[num_particulas].v=0;
                particulas[num_particulas].x=j+0.25f+((float) rand() / RAND_MAX) * 0.2f - 0.1f;
                particulas[num_particulas].y=i+0.25f+((float) rand() / RAND_MAX) * 0.2f - 0.1f;
                num_particulas=num_particulas+1;

                if (num_particulas >= MAX_PARTICULAS) return;
                particulas[num_particulas].u=0;
                particulas[num_particulas].v=0;
                particulas[num_particulas].x=j+0.75f+((float) rand() / RAND_MAX) * 0.2f - 0.1f;
                particulas[num_particulas].y=i+0.25f+((float) rand() / RAND_MAX) * 0.2f - 0.1f;
                num_particulas=num_particulas+1;

                if (num_particulas >= MAX_PARTICULAS) return;
                particulas[num_particulas].u=0;
                particulas[num_particulas].v=0;
                particulas[num_particulas].x=j+0.25f+((float) rand() / RAND_MAX) * 0.2f - 0.1f;
                particulas[num_particulas].y=i+0.75f+((float) rand() / RAND_MAX) * 0.2f - 0.1f;
                num_particulas=num_particulas+1;

                if (num_particulas >= MAX_PARTICULAS) return;
                particulas[num_particulas].u=0;
                particulas[num_particulas].v=0;
                particulas[num_particulas].x=j+0.75f+((float) rand() / RAND_MAX) * 0.2f - 0.1f;
                particulas[num_particulas].y=i+0.75f+((float) rand() / RAND_MAX) * 0.2f - 0.1f;
                num_particulas=num_particulas+1;
            }
        }
    }
}

/*
    Esta funcion marca que casillas tienen agua en la pantalla, a travez de la matriz de particulas
*/
void marcar_casillas_con_agua(void){
    int i, j, k;
    int fila, columna;

    //Todo lo no solido se vuelve aire
    for (i = 0; i < FILAS; i++) {
        for (j = 0; j < COLUMNAS; j++) {
            if (casillas[i][j].solido == 0) {
                casillas[i][j].tipo = AIRE;
                leds[i][j] = AIRE;
            }
        }
    }

    /* 2. Marcar la casilla de cada particula */
    for (k = 0; k < num_particulas; k++) {
        fila    = (int) particulas[k].y;
        columna = (int) particulas[k].x;

        if (fila < 0 || fila >= FILAS || columna < 0 || columna >= COLUMNAS)
            continue;
        if (casillas[fila][columna].solido == 1)
            continue;

        casillas[fila][columna].tipo = AGUA;
        leds[fila][columna] = AGUA;
    }
}

/*
    Esta funcion determina la velocidad y movimiento de las particulas a travez de la aceleracion de la gravedad
*/
void movimiento_particulas(void){
    int i;
    float temp=0;
    for(i=0; i<num_particulas;i++){
        particulas[i].x_ant = particulas[i].x;
        particulas[i].y_ant = particulas[i].y;

        particulas[i].u+=gx*Dt;
        particulas[i].v+=gy*Dt;

        temp=particulas[i].x;
        particulas[i].x+=particulas[i].u*Dt;
        if (casillas[(int) particulas[i].y][(int) particulas[i].x].solido == 1) {
            particulas[i].x = temp;
            particulas[i].u = 0;
        }

        temp=particulas[i].y;
        particulas[i].y+=particulas[i].v*Dt;
        if (casillas[(int) particulas[i].y][(int) particulas[i].x].solido == 1) {
            particulas[i].y = temp;
            particulas[i].v = 0;
        }
    }
}

//CLAUDE
void separar_particulas(void){
    int i, j;
    float dx, dy, d2, d, empuje;
    float xi, yi, xj, yj;

    for (i = 0; i < num_particulas; i++) {
        for (j = i + 1; j < num_particulas; j++) {
            dx = particulas[j].x - particulas[i].x;
            dy = particulas[j].y - particulas[i].y;
            d2 = dx * dx + dy * dy;

            if (d2 >= DIST_MIN * DIST_MIN || d2 == 0) continue;

            d = sqrtf(d2);
            empuje = (DIST_MIN - d) / 2 * 0.5f;
            dx = dx / d * empuje;
            dy = dy / d * empuje;

            /* Guardar por si hay que deshacer */
            xi = particulas[i].x;  yi = particulas[i].y;
            xj = particulas[j].x;  yj = particulas[j].y;

            particulas[i].x -= dx;  particulas[i].y -= dy;
            particulas[j].x += dx;  particulas[j].y += dy;

            /* Si alguna quedo dentro de una pared, deshacer */
            int pared_i = casillas[(int) particulas[i].y][(int) particulas[i].x].solido == 1;
            int pared_j = casillas[(int) particulas[j].y][(int) particulas[j].x].solido == 1;

            if (pared_i && pared_j) {           /* las dos: deshacer ambas */
                particulas[i].x = xi;  particulas[i].y = yi;
                particulas[j].x = xj;  particulas[j].y = yj;
            }
            else if (pared_i) {                 /* solo i: j se mueve el doble */
                particulas[i].x = xi;  particulas[i].y = yi;
                particulas[j].x += dx; particulas[j].y += dy;
                if (casillas[(int) particulas[j].y][(int) particulas[j].x].solido == 1) {
                    particulas[j].x = xj;  particulas[j].y = yj;
                }
            }
            else if (pared_j) {                 /* solo j: i se mueve el doble */
                particulas[j].x = xj;  particulas[j].y = yj;
                particulas[i].x -= dx; particulas[i].y -= dy;
                if (casillas[(int) particulas[i].y][(int) particulas[i].x].solido == 1) {
                    particulas[i].x = xi;  particulas[i].y = yi;
                }
            }
        }
    }
}

void P2G(void){
    int i, j, col,fila;
    float dx,dy;
    float peso_arri, peso_arrd, peso_abi, peso_abd;
    
    for(i=0;i<FILAS;i++){
        for(j=0;j<COLUMNAS;j++){
            casillas[i][j].u=0;
            casillas[i][j].v=0;
            casillas[i][j].peso_u=0;
            casillas[i][j].peso_v=0;
        }
    }

    for (i = 0; i < num_particulas; i++) {
        //Componente u
        //Convertir coordenadas de la particula a su cara correspondiente de la casilla
        col= (int) particulas[i].x;
        fila= (int) (particulas[i].y-0.5f);

        //Que tan lejos está la particula de la cara
        dx=particulas[i].x-col;
        dy=(particulas[i].y-0.5f)-fila;

        //Pesos del efecto de la particula en cada cara
        peso_arri=(1-dx)*(1-dy);
        peso_arrd=(dx)*(1-dy);
        peso_abi=(1-dx)*(dy);
        peso_abd=(dx)*(dy);

        casillas[fila][col].u          += particulas[i].u * peso_arri;
        casillas[fila][col].peso_u     += peso_arri;

        casillas[fila][col+1].u        += particulas[i].u * peso_arrd;
        casillas[fila][col+1].peso_u   += peso_arrd;

        casillas[fila+1][col].u        += particulas[i].u * peso_abi;
        casillas[fila+1][col].peso_u   += peso_abi;

        casillas[fila+1][col+1].u      += particulas[i].u * peso_abd;
        casillas[fila+1][col+1].peso_u += peso_abd;

        //Componente v
        //Convertir coordenadas de la particula a su cara correspondiente de la casilla
        col= (int) (particulas[i].x-0.5f);
        fila= (int) particulas[i].y;

        //Que tan lejos está la particula de la cara
        dx=(particulas[i].x-0.5f)-col;
        dy=particulas[i].y-fila;

        //Pesos del efecto de la particula en cada cara
        peso_arri=(1-dx)*(1-dy);
        peso_arrd=(dx)*(1-dy);
        peso_abi=(1-dx)*(dy);
        peso_abd=(dx)*(dy);

        casillas[fila][col].v          += particulas[i].v * peso_arri;
        casillas[fila][col].peso_v     += peso_arri;

        casillas[fila][col+1].v        += particulas[i].v * peso_arrd;
        casillas[fila][col+1].peso_v   += peso_arrd;

        casillas[fila+1][col].v        += particulas[i].v * peso_abi;
        casillas[fila+1][col].peso_v   += peso_abi;

        casillas[fila+1][col+1].v      += particulas[i].v * peso_abd;
        casillas[fila+1][col+1].peso_v += peso_abd;
    }

    for (i = 0; i < FILAS; i++) {
        for (j = 0; j < COLUMNAS; j++) {
            // Paso 4: convertir la suma en promedio
            if (casillas[i][j].peso_u > 0)
                casillas[i][j].u /= casillas[i][j].peso_u;
            if (casillas[i][j].peso_v > 0)
                casillas[i][j].v /= casillas[i][j].peso_v;

            // Paso 5: guardar la velocidad antes de la presion
            casillas[i][j].u_prev = casillas[i][j].u;
            casillas[i][j].v_prev = casillas[i][j].v;
        }
    }
}

/*
    Esta funcion hace que el liquido no se pueda salir hacia las casillas solidas
*/
void condiciones_de_frontera(void){
    int i,j;
    for (i = 1; i < FILAS; i++) {
        for (j = 1; j < COLUMNAS; j++) {
            if (casillas[i][j].solido == 1 ||casillas[i][j-1].solido==1){
                casillas[i][j].u=0;
            }
            if (casillas[i][j].solido == 1 ||casillas[i-1][j].solido==1){
                casillas[i][j].v=0;
            }
        }
    }
}
/*
    Esta funcion 
*/
void presion(void){
    int i,j,k;
    float s_izq, s_der, s_arr, s_abj, s_total, div, p;
    for (k=0;k<30;k++){
        for (i = 0; i < FILAS; i++) {
            for (j = 0; j < COLUMNAS; j++) {
                if (casillas[i][j].tipo==AGUA){
                    s_izq = 1 - casillas[i][j-1].solido;
                    s_der = 1 - casillas[i][j+1].solido;
                    s_arr = 1 - casillas[i-1][j].solido;
                    s_abj = 1 - casillas[i+1][j].solido;
                    s_total = s_izq + s_der + s_arr + s_abj;
                    if (s_total == 0) continue;

                    //Divergencia
                    div = (casillas[i][j+1].u - casillas[i][j].u) + (casillas[i+1][j].v - casillas[i][j].v);

                    p = -div / s_total * 1.9f;

                    casillas[i][j].u   -= s_izq * p;   // izquierda
                    casillas[i][j+1].u += s_der * p;   // derecha
                    casillas[i][j].v   -= s_arr * p;   // arriba
                    casillas[i+1][j].v += s_abj * p;   // abajo
                }
            }
        }
    }
}

#define FLIP 0.9f   /* 0.9 = 90% FLIP, 10% PIC */
/*
    CLAUDE
*/
void G2P(void){
    int i, col, fila;
    float dx, dy, suma, pic, cambio;
    float peso_arri, peso_arrd, peso_abi, peso_abd;

    for (i = 0; i < num_particulas; i++) {

        //Componente u
        col  = (int) particulas[i].x;
        fila = (int) (particulas[i].y - 0.5f);

        dx = particulas[i].x - col;
        dy = (particulas[i].y - 0.5f) - fila;

        peso_arri = (1 - dx) * (1 - dy);
        peso_arrd = dx * (1 - dy);
        peso_abi  = (1 - dx) * dy;
        peso_abd  = dx * dy;

        //Anular caras que no tocan agua (cara u: casilla y su vecina izquierda)
        if (casillas[fila][col].tipo     != AGUA && casillas[fila][col-1].tipo   != AGUA) peso_arri = 0;
        if (casillas[fila][col+1].tipo   != AGUA && casillas[fila][col].tipo     != AGUA) peso_arrd = 0;
        if (casillas[fila+1][col].tipo   != AGUA && casillas[fila+1][col-1].tipo != AGUA) peso_abi  = 0;
        if (casillas[fila+1][col+1].tipo != AGUA && casillas[fila+1][col].tipo   != AGUA) peso_abd  = 0;

        suma = peso_arri + peso_arrd + peso_abi + peso_abd;

        if (suma > 0) {
            pic = (peso_arri * casillas[fila][col].u
                 + peso_arrd * casillas[fila][col+1].u
                 + peso_abi  * casillas[fila+1][col].u
                 + peso_abd  * casillas[fila+1][col+1].u) / suma;

            cambio = (peso_arri * (casillas[fila][col].u     - casillas[fila][col].u_prev)
                    + peso_arrd * (casillas[fila][col+1].u   - casillas[fila][col+1].u_prev)
                    + peso_abi  * (casillas[fila+1][col].u   - casillas[fila+1][col].u_prev)
                    + peso_abd  * (casillas[fila+1][col+1].u - casillas[fila+1][col+1].u_prev)) / suma;

            particulas[i].u = FLIP * (particulas[i].u + cambio) + (1 - FLIP) * pic;
        }

        //Componente v
        col  = (int) (particulas[i].x - 0.5f);
        fila = (int) particulas[i].y;

        dx = (particulas[i].x - 0.5f) - col;
        dy = particulas[i].y - fila;

        peso_arri = (1 - dx) * (1 - dy);
        peso_arrd = dx * (1 - dy);
        peso_abi  = (1 - dx) * dy;
        peso_abd  = dx * dy;

        //Anular caras que no tocan agua (cara v: casilla y su vecina de arriba)
        if (casillas[fila][col].tipo     != AGUA && casillas[fila-1][col].tipo   != AGUA) peso_arri = 0;
        if (casillas[fila][col+1].tipo   != AGUA && casillas[fila-1][col+1].tipo != AGUA) peso_arrd = 0;
        if (casillas[fila+1][col].tipo   != AGUA && casillas[fila][col].tipo     != AGUA) peso_abi  = 0;
        if (casillas[fila+1][col+1].tipo != AGUA && casillas[fila][col+1].tipo   != AGUA) peso_abd  = 0;

        suma = peso_arri + peso_arrd + peso_abi + peso_abd;

        if (suma > 0) {
            pic = (peso_arri * casillas[fila][col].v
                 + peso_arrd * casillas[fila][col+1].v
                 + peso_abi  * casillas[fila+1][col].v
                 + peso_abd  * casillas[fila+1][col+1].v) / suma;

            cambio = (peso_arri * (casillas[fila][col].v     - casillas[fila][col].v_prev)
                    + peso_arrd * (casillas[fila][col+1].v   - casillas[fila][col+1].v_prev)
                    + peso_abi  * (casillas[fila+1][col].v   - casillas[fila+1][col].v_prev)
                    + peso_abd  * (casillas[fila+1][col+1].v - casillas[fila+1][col+1].v_prev)) / suma;

            particulas[i].v = FLIP * (particulas[i].v + cambio) + (1 - FLIP) * pic;
        }
    }
}
#define G          10.0f   /* magnitud de la gravedad */
#define VEL_GIRO    3.0f   /* radianes por segundo al mantener la flecha */

float angulo = 0.0f;       /* 0 = gravedad hacia abajo */

void leer_teclado(void){
    if (GetAsyncKeyState(VK_LEFT)  & 0x8000) angulo += VEL_GIRO * Dt;
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) angulo -= VEL_GIRO * Dt;
    if (GetAsyncKeyState(VK_UP)    & 0x8000) angulo = 0.0f;

    gx = G * sinf(angulo);
    gy = G * cosf(angulo);

    /* Sacudida: una gravedad fuerte hacia arriba durante este frame */
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
        gy = -3 * G;
    }
}