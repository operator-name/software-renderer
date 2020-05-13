with import <nixpkgs> {};

stdenv.mkDerivation rec {
  name = "software-renderer";
  src = ./.;

  buildInputs = with pkgs; [
    gcc_debug
    gdb
    clang_9 # clang-format
    clang-tools # clang-format
    meshlab
    nemiver

    SDL2

    ffmpeg
    mpv
  ];

  buildPhase = ''
    make
  '';

  installPhase = ''
    mkdir -p $out/bin/
    cp ${name}.out $out/bin/${name}
  '';
}
