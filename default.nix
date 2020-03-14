with import <nixpkgs> {};

stdenv.mkDerivation rec {
  name = "software-renderer";
  src = ./.;

  buildInputs = with pkgs; [
    # gdb
    clang_9 # clang-format
    clang-tools
    SDL2
  ];

  buildPhase = "make";

  installPhase = ''
    mkdir -p $out/bin/
    cp ${name}.out $out/bin/${name}
  '';
}