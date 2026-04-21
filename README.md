# pmr (PNG Metadata Reader)

Um parser simples escrito em C++23 para ler metadados ocultos em arquivos PNG. Ele navega pela estrutura binária do arquivo, ignorando os dados de pixel, para extrair os chunks de texto e tempo.

## O que ele extrai
- `tEXt`: Metadados em texto puro delimitados por nulos.
- `tIME`: Timestamp de modificação/criação original da imagem.
- `zTXt`: Metadados de texto comprimidos com DEFLATE.
- `iTXt`: Texto internacionalizado (UTF-8), com suporte a dados puros ou comprimidos.

## Dependências
- Compilador compatível com C++23 (faz uso de `<print>`)
- `CMake`
- `zlib` (necessária para descomprimir os chunks zTXt e iTXt)

*Instalação no Arch Linux:*
`sudo pacman -S zlib cmake`

## Como compilar e usar

**Build:**
```bash
cmake -S . -B build
cmake --build build
```

## Uso:
Passe o caminho da imagem como argumento:

```bash
./build/pmr arquivo.png
```

## Como funciona

O código faz o parser manual do formato PNG (RFC 2083) usando chamadas padrão de I/O em C.
Ele resolve o Endianness dos blocos dinamicamente e calcula os offsets para pular arquivos grandes (IDAT) direto no disco.
A descompressão aloca buffers locais usando smart pointers (std::make_unique) e repassa a carga matemática para a função uncompress da biblioteca padrão zlib.
