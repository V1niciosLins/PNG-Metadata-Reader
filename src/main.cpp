#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory.h>
#include <memory>
#include <print>
#include <zlib.h>

// Sei que não é boa prátca misturar C com C++, e eu concordaria se isto aqui
// fosse sério ou eu estivesse em uma equipe, nenhum desses é real, logo...
uint8_t MAGIC_BYTES[]{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

typedef struct PNG_META_DATA {
  uint32_t width{}, height{};
} PNG_META_DATA;

namespace iChunks {
constexpr uint32_t tEXt = 0x74455874;
constexpr uint32_t tIME = 0x74494D45;
constexpr uint32_t zTXt = 0x7A545874;
constexpr uint32_t iTXt = 0x69545874;
} // namespace iChunks

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::print(stderr,
               "Quantidade de argumentos inválidos. Use: ./pmr <file_path>");
    return -1;
  }
  char *PNG_PATH{argv[1]};
  FILE *f = fopen(PNG_PATH, "rb");

  if (!f) {
    std::print("Deu pra abrir nn mano...");
    return -1;
  }

  uint8_t magic_bytes_buffer[8]{};

  fread(magic_bytes_buffer, 1, sizeof(magic_bytes_buffer), f);

  for (int x = 0; x < 8; ++x)
    if (MAGIC_BYTES[x] != magic_bytes_buffer[x]) {
      std::print("Isso ai n é um png nn mano...");
      return -1;
    }
  // Blz ate aqui ta td certo.
  //
  /**
   * ANATOMIA ESTRUTURAL DO ARQUIVO PNG (Pra eu não perder o foco saindo daqui
   * pra documentação toda hora)
   * ---------------------------------
   * O arquivo é composto por uma ASSINATURA fixa seguida por CHUNKS lineares.
   * * [1] ASSINATURA (Offset 0-7): 8 bytes fixos
   * Hex: 89 50 4E 47 0D 0A 1A 0A  (MAGIC_BYTES)
   *
   * [2] ESTRUTURA UNIVERSAL DE UM CHUNK (Vagão):
   * +---------------------------------------------------------------+
   * | Campo  | Tamanho | Descrição                                  |
   * +--------+---------+--------------------------------------------+
   * | Length | 4 bytes | Tam. do campo DATA (Big-Endian/Byteswap)   |
   * | Type   | 4 bytes | Nome em ASCII (Ex: "IHDR", "tEXt")         |
   * | Data   | L bytes | O conteúdo real (variável conforme Length) |
   * | CRC    | 4 bytes | Checksum do Type + Data (Big-Endian)       |
   * +--------+---------+--------------------------------------------+
   * Nota: Pulo total para o próximo chunk = Length + 12 bytes.
   * Apenas  os primeiros MAGIC_BYTES e os chunks IHDR e IEND tem suas posições
   * e tamanhos garantidos, todos os outros podem ou não estar lá, em uma ordem
   * 'aleatória' ou não e com seu vagão DATA de tamanho N
   *
   * [3] DETALHAMENTO DO CHUNK IHDR (Offset 8-32):
   * É o primeiro chunk e possui 13 bytes de DATA obrigatórios:
   * - Data[0-3] : Width (4 bytes, uint32_t, Big-Endian)
   * - Data[4-7] : Height (4 bytes, uint32_t, Big-Endian)
   * - Data[8]   : Bit Depth (1 byte)
   * - Data[9]   : Color Type (1 byte)
   * - Data[10]  : Compression Method (1 byte, deve ser 0)
   * - Data[11]  : Filter Method (1 byte, deve ser 0)
   * - Data[12]  : Interlace Method (1 byte)
   *
   * [4] LÓGICA DE BUSCA (While Loop):
   * Como a ordem (exceto IHDR/IEND) não é garantida, a estratégia é:
   * 1. Ler 4 bytes (Length) -> Byteswap
   * 2. Ler 4 bytes (Type)
   * 3. Comparar Type com alvo (ex: "tEXt")
   * 4. Se não for, fseek(Length + 4) para pular Data + CRC e ler próximo.
   *
   *
   *
   *
   * ANATOMIA DA CARGA (DATA) - ESCOPO DE METADADOS
   * O tamanho total descrito abaixo é sempre o valor do campo 'Length'.
   *
   * -------------------------------------------------------------------
   * 1. tEXt (Texto Puro)
   * O mais simples. Dividido em dois pelo primeiro byte nulo (0x00).
   * -------------------------------------------------------------------
   * - [Tamanho Variável] : Keyword (Chave). ISO 8859-1. (Ex: "Author")
   * - [1 byte cravado]   : Separador Nulo (Valor obrigatório: 0x00)
   * - [O que sobrar]     : Text (Valor). ISO 8859-1 até acabar o Length.
   * * -------------------------------------------------------------------
   * 2. zTXt (Texto Comprimido)
   * Igual ao tEXt, mas o valor está esmagado em zlib.
   * -------------------------------------------------------------------
   * - [Tamanho Variável] : Keyword (Chave). ISO 8859-1.
   * - [1 byte cravado]   : Separador Nulo (Valor: 0x00)
   * - [1 byte cravado]   : Compression Method (Sempre 0x00 para zlib)
   * - [O que sobrar]     : Compressed Text (Binário bruto zlib)
   * * -------------------------------------------------------------------
   * 3. iTXt (Texto Internacional)
   * Suporta UTF-8. Múltiplos delimitadores nulos na estrutura.
   * -------------------------------------------------------------------
   * - [Tamanho Variável] : Keyword (Chave). UTF-8.
   * - [1 byte cravado]   : Separador Nulo (Valor: 0x00)
   * - [1 byte cravado]   : Compression Flag (0 = Puro, 1 = Comprimido zlib)
   * - [1 byte cravado]   : Compression Method (Sempre 0x00)
   * - [Tamanho Variável] : Language Tag (Ex: "pt-BR"). Termina no próximo 0x00.
   * - [1 byte cravado]   : Separador Nulo (Valor: 0x00)
   * - [Tamanho Variável] : Translated Keyword. Termina no próximo 0x00.
   * - [1 byte cravado]   : Separador Nulo (Valor: 0x00)
   * - [O que sobrar]     : Text (O texto real UTF-8 ou o bloco zlib).
   *
   * -------------------------------------------------------------------
   * 4. tIME (Carimbo de Tempo)
   * Não tem texto, não tem nulo. São sempre exatos 7 bytes de tamanho.
   * -------------------------------------------------------------------
   * - Byte [0-1] : Ano (uint16_t, Big-Endian -> Precisa de std::byteswap)
   * - Byte [2]   : Mês (1-12)
   * - Byte [3]   : Dia (1-31)
   * - Byte [4]   : Hora (0-23)
   * - Byte [5]   : Minuto (0-59)
   * - Byte [6]   : Segundo (0-60)
   *   [3]/[2]/[0-1] - [4]:[5]:[6]
   */
  //
  //
  PNG_META_DATA png_md;

  uint8_t IDHR_CHUNK[25]{};
  fread(IDHR_CHUNK, 1, sizeof(IDHR_CHUNK), f);

  png_md.width = std::byteswap(*reinterpret_cast<uint32_t *>(&IDHR_CHUNK[8]));

  png_md.height = std::byteswap(*reinterpret_cast<uint32_t *>(&IDHR_CHUNK[12]));

  std::println("A imagem tem {} de largura e {} de altura", png_md.width,
               png_md.height);

  while (1) {
    uint8_t datal_and_type[8]{};
    fread(datal_and_type, 1, sizeof(datal_and_type), f);

    uint32_t type =
        std::byteswap(*reinterpret_cast<uint32_t *>(&datal_and_type[4]));
    uint32_t datal =
        std::byteswap(*reinterpret_cast<uint32_t *>(&datal_and_type[0]));

    if (type == 0x49454E44) {
      std::println("Fim do arquivo, Debug: type={:x}", type);
      break;
    }

    if (type != iChunks::tEXt && type != iChunks::iTXt &&
        type != iChunks::tIME && type != iChunks::zTXt) {
      fseek(f, datal + 4, SEEK_CUR);
      continue;
    }

    char *chunk_data = new char[datal + 1];
    fread(chunk_data, 1, datal, f);
    chunk_data[datal] = '\0';

    switch (type) {

    case iChunks::tEXt: {
      chunk_data[strlen(chunk_data)] = ':';
      std::println("Metadados do chunk tEXt: {:?}", chunk_data);
      break;
    }
    case iChunks::tIME: {
      //[3]/[2]/[0-1] - [4]:[5]:[6]

      uint16_t year{std::byteswap(*reinterpret_cast<uint16_t *>(chunk_data))};
      uint8_t day = chunk_data[3];
      uint8_t month = chunk_data[2];

      uint8_t hour = chunk_data[4];
      uint8_t minute = chunk_data[5];
      uint8_t seconds = chunk_data[6];

      std::println("Campo tIME: {:02}/{:02}/{:04} - {:02}:{:02}:{:02}", day,
                   month, year, hour, minute, seconds);
      break;
    }

    case iChunks::zTXt: {

      /* -------------------------------------------------------------------
       * - [Tamanho Variável] : Keyword (Chave). ISO 8859-1.
       * - [1 byte cravado]   : Separador Nulo (Valor: 0x00)
       * - [1 byte cravado]   : Compression Method (Sempre 0x00 para zlib)
       * - [O que sobrar]     : Compressed Text (Binário bruto zlib)
       * * -------------------------------------------------------------------
       * */
      std::println("Lendo zTXt: ");
      std::println("{} ", chunk_data);
      // 0x00 pra separar e depois outro 0x00 pra metodo de compressão;
      //
      const unsigned char *zlib_data = reinterpret_cast<const unsigned char *>(
          &chunk_data[strlen(chunk_data) + 2]);
      size_t len = (datal - (strlen(chunk_data) + 2));
      uLongf dest_len = len * 100; // chute
      auto dest_buf = std::make_unique<char[]>(dest_len);

      int z_status = uncompress(reinterpret_cast<Bytef *>(dest_buf.get()),
                                &dest_len, zlib_data, len);

      if (z_status == Z_OK) {
        dest_buf[dest_len] = '\0';
        std::println("Texto Descomprimido: {}", dest_buf.get());
      } else {
        std::println("Erro na descompressão zlib. Código: {}", z_status);
      }

      break;
    }
      /*      [Tamanho Variável] : Keyword (Chave). UTF-8.
       * - [1 byte cravado]   : Separador Nulo (Valor: 0x00)
       * - [1 byte cravado]   : Compression Flag (0 = Puro, 1 = Comprimido zlib)
       * - [1 byte cravado]   : Compression Method (Sempre 0x00)
       * - [Tamanho Variável] : Language Tag (Ex: "pt-BR"). Termina no próximo
       * 0x00.
       * - [1 byte cravado]   : Separador Nulo (Valor: 0x00)
       * - [Tamanho Variável] : Translated Keyword. Termina no próximo 0x00.
       * - [1 byte cravado]   : Separador Nulo (Valor: 0x00)
       * - [O que sobrar]     : Text (O texto real UTF-8 ou o bloco zlib).
       */
      // Primeiro a kayword, vai parar no primeiro 0x00;
    case iChunks::iTXt: {
      std::println("Lendo iTXt: ");

      size_t pos = 0;

      std::println("Keyword: {}", &chunk_data[pos]);
      pos += strlen(&chunk_data[pos]) + 1;

      if (pos + 2 > datal)
        break;

      bool is_compressed = chunk_data[pos++];
      pos++; // Vai ser sempre 0 mesmo, vou ler pq?

      std::println("Language Tag: {}", &chunk_data[pos]);
      pos += strlen(&chunk_data[pos]) + 1;

      std::println("Translated Keyword: {}", &chunk_data[pos]);
      pos += strlen(&chunk_data[pos]) + 1;

      if (!is_compressed) {

        std::println("Texto Final: {}", &chunk_data[pos]);
      } else {

        std::println(
            "O Texto Final está comprimido! (Iniciando descompressão...)");

        size_t zlib_len = datal - pos;
        const unsigned char *zlib_data =
            reinterpret_cast<const unsigned char *>(&chunk_data[pos]);

        uLongf dest_len = zlib_len * 100; // chute
        auto dest_buf = std::make_unique<char[]>(dest_len);

        int z_status = uncompress(reinterpret_cast<Bytef *>(dest_buf.get()),
                                  &dest_len, zlib_data, zlib_len);

        if (z_status == Z_OK) {
          dest_buf[dest_len] = '\0';
          std::println("Texto Descomprimido: {}", dest_buf.get());
        } else {
          std::println("Erro na descompressão zlib. Código: {}", z_status);
        }
      }
      break;
    }

      // TODO: Ler outros tipos de chunks de metadados;
    }

    delete[] chunk_data;

    fseek(f, 4, SEEK_CUR);
  }

  fclose(f);

  return 0;
}
