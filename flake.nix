{
  description = "Coroutines in C";
  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-24.11";
  outputs = { self, nixpkgs }: let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};

    shell = { pkgs }: let
      isWindows = builtins.match ".*-windows" pkgs.stdenv.hostPlatform.system != null;
    in pkgs.mkShell {
      nativeBuildInputs = with pkgs.buildPackages; [
        cmake
        gdb
      ];
      shellHook = pkgs.lib.concatStringsSep "\n" (
        pkgs.lib.optional isWindows ''
          mkdir build-win64
          cd build-win64
          cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/x86_64-w64-mingw32.cmake -DCMAKE_BUILD_TYPE=Debug ../
          cp -u ${pkgs.windows.mcfgthreads}/bin/*mcfgthread*.dll .
        ''
      );
    };

  in {
    devShells.${system} = {
      win64 = shell { pkgs = pkgs.pkgsCross.mingwW64; };
      win32 = shell { pkgs = pkgs.pkgsCross.mingw32; };
    };
  };
}
