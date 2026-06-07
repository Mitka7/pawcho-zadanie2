# syntax=docker/dockerfile:1
# Dyrektywa określająca wersję składni BuildKit niezbędną do obsługi zaawansowanych funkcji (np. --mount).

# ETAP 1: Środowisko budujące (wykorzystanie Alpine z kompilatorem gcc i biblioteką musl)
# - działające natywnie na architekturze hosta (--platform=$BUILDPLATFORM)
FROM --platform=$BUILDPLATFORM alpine:latest AS builder

# Instalacja narzędzi do pobierania kodu (Git/SSH) oraz optymalizacji binarnej (UPX)
# clang i lld są wymagane przez 'xx' do sprawnej kompilacji międzyplatformowej
RUN apk add --no-cache clang lld git openssh-client upx

# Instalacja 'xx' - profesjonalnych skryptów pomocniczych do kompilacji skrośnej
COPY --from=tonistiigi/xx / /

# Argumenty BuildKit niezbędne do poprawnej obsługi kompilacji skrośnej
ARG TARGETPLATFORM

# Narzędzie xx-apk automatycznie pobierze 'musl-dev' dla amd64 oraz arm64
RUN xx-apk add --no-cache musl-dev gcc

# Katalog roboczy
WORKDIR /app

# Konfiguracja SSH dla GitHuba (keyscan zapobiega interaktywnym pytaniom o zaufany host)
RUN mkdir -p -m 0700 ~/.ssh && ssh-keyscan github.com >> ~/.ssh/known_hosts

# Pobranie kodu aplikacji bezpośrednio z lokalnego katalogu repozytorium 
COPY server.c .


# Kompilacja KRZYŻOWA (Cross-compilation) za pomocą xx-clang
# -Os: optymalizacja pod rozmiar pliku (wyłącz te optymalizacje, które zwiększają rozmiar pliku)
# -static: statyczne linkowanie (wymóg dla obrazu scratch)
# -ffunction-sections -fdata-sections -Wl,--gc-sections: usuwanie nieużywanego kodu z binarki
# upx --best --lzma: kompresowanie gotowego pliku binarnego do absolutnego minimum
RUN xx-clang -Os -static -s -ffunction-sections -fdata-sections -Wl,--gc-sections server.c -o server && \
    xx-verify server && \
    upx --best --lzma server

# ETAP 2: Docelowy obraz
FROM scratch

# Informacje o autorze zgodne ze standardem OCI
LABEL org.opencontainers.image.source="https://github.com/Mitka7/pawcho-zadanie1"
LABEL org.opencontainers.image.description="Serwer WWW pogody (Wersja Multi-arch + SSH)"
LABEL org.opencontainers.image.authors="Weronika Mitaszka <s101631@pollub.edu.pl>"


# Kopiowanie skompresowanego pliku - jedyna fizyczna warstwa obrazu
COPY --from=builder /app/server /server

# Użytkownik o numerze ID 10001 (nie-root)
USER 10001

# Dokumentacja nasłuchującego portu
EXPOSE 8080/tcp

# Healthcheck. Obraz scratch nie ma shella, stąd wywołanie aplikacji
HEALTHCHECK --interval=30s --timeout=3s --retries=3 \
    CMD ["/server", "health"]

# Uruchomienie aplikacji
CMD ["/server"]
