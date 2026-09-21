case "$(uname -m)" in
  aarch64|arm64) export CONNEXTDDS_ARCH=armv8Linux4gcc8.5.0 ;;
  x86_64|amd64) export CONNEXTDDS_ARCH=x64Linux4gcc8.5.0 ;;
  *) echo "Unsupported Connext architecture: $(uname -m)" >&2; return 1 ;;
esac
