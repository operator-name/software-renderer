with import <nixpkgs> {};

stdenv.mkDerivation rec {
  name = "software-renderer";
  src = ./.;

  buildInputs = with pkgs; [
    SDL2 # dependancy
    
    clang-tools # clang-format
    
    # debugging
    valgrind
    gcc_debug
    gdb
    nemiver


    meshlab
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
