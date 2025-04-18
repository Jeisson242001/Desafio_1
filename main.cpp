#include <fstream>
#include <iostream>
#include <QCoreApplication>
#include <QImage>

using namespace std;

// FUNCIONES ORIGINALES DE ANIBAL ¡¡¡NO SE TOCAN!!!!
unsigned char* loadPixels(QString input, int &width, int &height) {
    QImage imagen(input);

    if (imagen.isNull()) {
        cout << "Error: No se pudo cargar la imagen BMP." << endl;
        return nullptr;
    }
    imagen = imagen.convertToFormat(QImage::Format_RGB888);
    width = imagen.width();
    height = imagen.height();
    int dataSize = width * height * 3;
    unsigned char* pixelData = new unsigned char[dataSize];
    for (int y = 0; y < height; ++y) {
        const uchar* srcLine = imagen.scanLine(y);
        unsigned char* dstLine = pixelData + y * width * 3;
        memcpy(dstLine, srcLine, width * 3);
    }
    return pixelData;
}

bool exportImage(unsigned char* pixelData, int width, int height, QString archivoSalida) {
    QImage outputImage(width, height, QImage::Format_RGB888);
    for (int y = 0; y < height; ++y) {
        memcpy(outputImage.scanLine(y), pixelData + y * width * 3, width * 3);
    }
    if (!outputImage.save(archivoSalida, "BMP")) {
        cout << "Error: No se pudo guardar la imagen BMP modificada." << endl;
        return false;
    } else {
        cout << "Imagen BMP modificada guardada como " << archivoSalida.toStdString() << endl;
        return true;
    }
}

unsigned int* loadSeedMasking(const char* nombreArchivo, int &seed, int &n_pixels) {
    ifstream archivo(nombreArchivo);
    if (!archivo.is_open()) {
        cout << "No se pudo abrir el archivo." << endl;
        return nullptr;
    }
    archivo >> seed;
    int r, g, b;
    while (archivo >> r >> g >> b) {
        n_pixels++;
    }
    archivo.close();
    archivo.open(nombreArchivo);
    if (!archivo.is_open()) {
        cout << "Error al reabrir el archivo." << endl;
        return nullptr;
    }
    unsigned int* RGB = new unsigned int[n_pixels * 3];
    archivo >> seed;
    for (int i = 0; i < n_pixels * 3; i += 3) {
        archivo >> r >> g >> b;
        RGB[i] = r;
        RGB[i + 1] = g;
        RGB[i + 2] = b;
    }
    archivo.close();
    cout << "\nSemilla: " << seed << endl;
    cout << "Cantidad de pixeles leidos: " << n_pixels << endl;
    return RGB;
}

// FUNCIONES A NIVEL DE BITS Y DEMAS MIAS, MUEVALAS COMO QUIERA PERO CON CUIDADO


//             FUNCION PARA XOR
uint8_t xorBytes(uint8_t a, uint8_t b) {
    return a ^ b;
}

//             FUNCION PARA DESPLAZAR BITS A LA IZQUIERDA
uint8_t despla_izquierda(uint8_t value, unsigned int bits) {
    return (bits <= 8) ? (value << bits) : value;
}

//             FUNCION PARA DESPLAZAR BITS A LA DERECHA
uint8_t despla_derecha(uint8_t value, unsigned int bits) {
    return (bits <= 8) ? (value >> bits) : value;
}

//             FUNCION PARA ROTAR BITS A LA IZQUIERDA
uint8_t rota_izquierda(uint8_t value, unsigned int bits) {
    bits %= 8;
    return ((value << bits) | (value >> (8 - bits))) & 0xFF;
}

//             FUNCION PARA ROTAR BITS A LA DERECHA
uint8_t rota_derecha(uint8_t value, unsigned int bits) {
    bits %= 8;
    return ((value >> bits) | (value << (8 - bits))) & 0xFF;
}

//             FUNCION PARA DESENMASCARAR  (MODIFICA EL ARREGLO DE PIXELES, LO EDITA DIRECTAMENTE)
void undoMasking(unsigned char* imagen, int width, int height,
                 unsigned int* sumaRGB, int seed, int n_pixels,
                 unsigned char* mascara) {
    for (int k = 0; k < n_pixels; ++k) {
        int idxMascara = k * 3;
        int idxImagen = (k + seed) * 3;
        if (idxImagen + 2 >= width * height * 3) continue;
        for (int c = 0; c < 3; ++c) {
            int valor = static_cast<int>(sumaRGB[idxMascara + c]) - static_cast<int>(mascara[idxMascara + c]);
            valor = max(0, min(255, valor));
            imagen[idxImagen + c] = static_cast<unsigned char>(valor);
        }
    }
}

//             ENTERO PARA VER CUANTAS DIFERENCIAS HAY ENTRE IMAGENES
int compararImagenes(unsigned char* img1, unsigned char* img2, int size) {
    int diferencias = 0;
    for (int i = 0; i < size; ++i) {
        if (img1[i] != img2[i]) diferencias++;
    }
    return diferencias;
}


//                                  MAIN COMPLETO
int main() {
    QString ruta = "C:/Users/LENOVO/OneDrive/Desktop/desafio 1/Prueba desafio/Caso 1/";
    int width, height;

    unsigned char* I_D = loadPixels(ruta + "I_D.bmp", width, height);
    unsigned char* I_M = loadPixels(ruta + "I_M.bmp", width, height);
    unsigned char* mascara = loadPixels(ruta + "M.bmp", width, height);

    if (!I_D || !I_M || !mascara) {
        cout << "Error al cargar las imagenes." << endl;
        return -1;
    }

    // Paso 1: desenmascarar con M2
    int seed2 = 0, n2 = 0;
    unsigned int* S2 = loadSeedMasking((ruta + "M2.txt").toStdString().c_str(), seed2, n2);
    if (!S2) return -1;
    undoMasking(I_D, width, height, S2, seed2, n2, mascara);
    delete[] S2;

    // Guardar como P2_reconstruido
    exportImage(I_D, width, height, ruta + "P2_reconstruido.bmp");

    // Comparar con P2 original
    unsigned char* P2_real = loadPixels(ruta + "P2.bmp", width, height);
    if (P2_real) {
        int difP2 = compararImagenes(I_D, P2_real, width * height * 3);
        cout << "Diferencias entre P2 reconstruido y P2 original: " << difP2 << endl;
        delete[] P2_real;
    }

    // Paso 2: XOR con I_M
    for (int i = 0; i < width * height * 3; ++i)
        I_D[i] = xorBytes(I_D[i], I_M[i]);

    // Guardar como P1_reconstruido
    exportImage(I_D, width, height, ruta + "P1_reconstruido.bmp");

    // Comparar con P1 original
    unsigned char* P1_real = loadPixels(ruta + "P1.bmp", width, height);
    if (P1_real) {
        int difP1 = compararImagenes(I_D, P1_real, width * height * 3);
        cout << "Diferencias entre P1 reconstruido y P1 original: " << difP1 << endl;
        delete[] P1_real;
    }

    // Paso 3: rotar bits a la izquierda
    for (int i = 0; i < width * height * 3; ++i)
        I_D[i] = rota_izquierda(I_D[i], 3);

    // Paso 4: desenmascarar con M1
    int seed1 = 0, n1 = 0;
    unsigned int* S1 = loadSeedMasking((ruta + "M1.txt").toStdString().c_str(), seed1, n1);
    if (!S1) return -1;
    undoMasking(I_D, width, height, S1, seed1, n1, mascara);
    delete[] S1;

    // Paso 5: XOR final con I_M
    for (int i = 0; i < width * height * 3; ++i)
        I_D[i] = xorBytes(I_D[i], I_M[i]);

    exportImage(I_D, width, height, ruta + "I_O_reconstruida.bmp");

    unsigned char* I_O = loadPixels(ruta + "I_O.bmp", width, height);
    if (I_O) {
        int diferencias = compararImagenes(I_D, I_O, width * height * 3);
        cout << "Diferencias entre imagen original y reconstruida: " << diferencias << endl;
        delete[] I_O;
    } else {
        cout << "No se pudo cargar la imagen original para comparar." << endl;
    }

    delete[] I_D;
    delete[] I_M;
    delete[] mascara;
    return 0;
}
