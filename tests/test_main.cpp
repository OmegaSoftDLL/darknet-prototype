// Ponto de entrada dos testes (doctest). Fica num TU SEPARADO dos casos de
// teste porque a implementacao do doctest inclui <windows.h> (mesmo com
// WIN32_LEAN_AND_MEAN), que declara CloseWindow/ShowCursor/Rectangle e
// conflita com a raylib. Aqui nao entra nenhum header do jogo.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
