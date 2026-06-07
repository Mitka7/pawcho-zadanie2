## Logowanie do GitHub 

<img width="945" height="213" alt="image" src="https://github.com/user-attachments/assets/761b362f-0670-4ee4-8a11-9298c3de6912" />

## Inicjalizacja lokalnego repozytorium

<img width="945" height="92" alt="image" src="https://github.com/user-attachments/assets/19add944-baa2-4962-8f32-d1c69854f62d" />

## Tworzenie repozytorium na GitHub

<img width="945" height="310" alt="image" src="https://github.com/user-attachments/assets/fb42f38b-d4dd-45d7-98ea-b04b25ca9b48" />

## Zarządzanie secretami i zmiennymi

<img width="944" height="281" alt="image" src="https://github.com/user-attachments/assets/ad8313d9-cfdf-4f96-8ae7-5405bcca4b48" />

## Zapisanie plików i wysłanie ich na GitHuba

<img width="939" height="614" alt="image" src="https://github.com/user-attachments/assets/9c87cfff-63c3-4b5d-9931-df1941c5a21c" />

## Inicjacja potoku GitHub Actions poprzez tagowanie repozytorium

<img width="945" height="294" alt="image" src="https://github.com/user-attachments/assets/81905a13-cb2c-4709-b316-0a0891875114" />

### Zrzut ekranu przedstawia proces nadania tagu v1.0.0 w lokalnym repozytorium oraz jego synchronizację z repozytorium zdalnym na platformie GitHub. Zgodnie z przyjętą konfiguracją pliku YAML, operacja ta pełni rolę wyzwalacza (triggera), który automatycznie uruchamia zdefiniowany łańcuch GitHub Actions.

## Weryfikacja działania potoku automatyzacji i skanowania bezpieczeństwa

<img width="945" height="586" alt="image" src="https://github.com/user-attachments/assets/96f88ff5-48fd-4c29-8249-f32ee3fd0c3c" />

### Zrzut ekranu (komenda: gh run watch) prezentuje poprawne zakończenie wszystkich zdefiniowanych kroków potoku. Widok potwierdza bezbłędne przejście krytycznych etapów zadania, w tym lokalnego skanowania podatności (Trivy vulnerability scanner) oraz pomyślne zbudowanie i wypchnięcie obrazu wieloarchitekturowego. Cały proces zakończył się ostatecznym statusem success.

## 1. Konfiguracja poszczególnych etapów zadania

### a. Wsparcie dla wielu architektur (linux/arm64 oraz linux/amd64)
Aby obraz wspierał architekturę ARM64 na standardowych (linuxowych) runnerach GitHuba, w łańcuchu wykorzystano emulator sprzętowy.
* **Konfiguracja:** Użyto akcji `docker/setup-qemu-action` (krok 3), która instaluje emulator QEMU. Następnie, w kroku 4 uruchomiono silnik `docker/setup-buildx-action`, który rozszerza możliwości standardowego Dockera o budowanie wieloarchitekturowe.
* **Realizacja:** W docelowym etapie budowania (krok 9), w akcji `docker/build-push-action` zadeklarowano parametr `platforms: linux/amd64,linux/arm64`, co wymusza jednoczesne skompilowanie kodu dla obu architektur.

### b. Wykorzystanie pamięci podręcznej (Cache) z DockerHub
Efemeryczność (ulotność) maszyn wirtualnych w GitHub Actions wymusiła zastosowanie zewnętrznego rejestru jako magazynu pamięci podręcznej.
* **Konfiguracja:** W krokach 7 oraz 9 zastosowano zewnętrzne źródło i cel dla pamięci podręcznej. Skonfigurowano backend typu `registry` wskazujący na dedykowane repozytorium w serwisie DockerHub: `type=registry,ref=<USERNAME>/pawcho-zadanie2-cache:max`.
* **Realizacja:** Aby zapewnić pełną użyteczność cache'u przy wieloetapowym pliku `Dockerfile` (multi-stage build z użyciem `xx-clang`), dodano flagę eksportu `mode=max`. Wypycha ona do chmury warstwy ze wszystkich etapów kompilacji, a nie tylko finalny obraz.

### c. Test CVE (Trivy) i blokowanie publikacji zagrożonych obrazów
Zgodnie z wymaganiem, obraz trafia do publicznego rejestru (GHCR) TYLKO w sytuacji braku krytycznych luk bezpieczeństwa. Zrealizowano to poprzez rozdzielenie procesu na dwa niezależne etapy budowania.
* **Konfiguracja:** W kroku 7 budowany jest lokalny, testowy obraz (`load: true`), który nie jest nigdzie publikowany, a jedynie ładowany do demona Dockera na runnerze. Następnie (krok 8) obraz ten skanowany jest przez skaner Trivy. Dopiero w kroku 9 (jeśli krok 8 zakończy się sukcesem) następuje właściwe budowanie multi-arch i eksport.
* **Wybór narzędzia (Dlaczego Trivy, a nie Docker Scout?):**
  Do realizacji zadania wybrano narzędzie **Trivy** firmy Aqua Security. Decyzja ta podyktowana była następującymi argumentami:
  1. **Prostota i natywna integracja (Fail-fast):** Trivy (jako `aquasecurity/trivy-action`) działa "out-of-the-box" bezpośrednio w środowisku CI bez konieczności integrowania zewnętrznych pulpitów analitycznych. Posiada wbudowany parametr `exit-code: '1'`, który automatycznie przerywa cały łańcuch w przypadku wykrycia zadeklarowanych luk (`severity: 'CRITICAL,HIGH'`).
  2. **Brak dodatkowych autoryzacji do API:** Docker Scout, choć potężny, do pełnej, zaawansowanej analizy rekomenduje uwierzytelnienie w ekosystemie Docker Desktop/Hub i przesyłanie tzw. *SBOM* (Software Bill of Materials). Trivy skanuje wygenerowany przez runnera lokalny plik z obrazem, co jest znacznie lżejszym i szybszym rozwiązaniem w prostych potokach CI/CD.
  * *Źródła:*
    * [Trivy GitHub Action - Trivy Documentation](https://aquasecurity.github.io/trivy-action/)
    * [Docker Documentation: Evaluating Trivy vs Scout considerations](https://docs.docker.com/scout/)

---

## 2. Architektura tagowania i zarządzania pamięcią podręczną (Cache)

Wdrożony łańcuch CI/CD wykorzystuje zaawansowane mechanizmy tagowania oraz optymalizacji pamięci podręcznej, opierając się na oficjalnych dokumentacjach i najlepszych praktykach środowisk chmurowych.

### Tagowanie obrazów produkcyjnych (GitHub Container Registry)
Dla docelowych obrazów w rejestrze `ghcr.io` zastosowano hybrydowy model tagowania (moduł `docker/metadata-action`), nadając obrazom dwa równoległe tagi:
* **Wersję semantyczną (SemVer):** (np. `v1.0.0`), wyciąganą bezpośrednio z tagów w systemie Git.
* **Krótki skrót z repozytorium (SHA):** (np. `sha-a1b2c3d`), gwarantujący technologiczną precyzję.

Dodatkowo celowo zablokowano automatyczne dodawanie domyślnego tagu `latest` (parametr `flavor: latest=false`).
* **Uzasadnienie architektoniczne:** Stosowanie tagu `latest` w środowiskach produkcyjnych jest uznawane za tzw. antywzorzec (anti-pattern), ponieważ jest to tag mutowalny, co może prowadzić do nieprzewidywalnych wdrożeń i trudności z wycofywaniem zmian. Oparcie architektury na unikalnych skrótach SHA zapewnia pełną niezmienność obrazu (immutability) i pozwala w ułamku sekundy powiązać działający kontener z konkretnym stanem kodu.
* **Źródła:**
  * Niezmienność tagów: [Docker Docs: Manage tags and labels](https://docs.docker.com/develop/dev-best-practices/#manage-tags-and-labels)
  * Antywzorzec tagu latest: [AWS Containers Roadmap: Tag Immutability #180](https://github.com/aws/containers-roadmap/issues/180)

### Strategia warstw Cache (DockerHub)
Dla pamięci podręcznej zastosowano w rejestrze stały tag tekstowy `:max` oraz parametr eksportu `mode=max`.
* **Uzasadnienie architektoniczne:** Zastosowanie parametru `mode=max` wymusza wyeksportowanie warstw ze *wszystkich* pośrednich etapów kompilacji, co maksymalizuje tzw. *cache hits* przy kolejnych uruchomieniach CI. Zastosowanie stałego tagu (`:max`) w docelowym rejestrze zapobiega zjawisku *registry bloat* (zaśmiecaniu repozytorium setkami nieużytecznych tagów poszczególnych warstw), ponieważ system nadpisuje i aktualizuje tylko stare manifesty.
* **Źródła:**
  * Optymalizacja Cache (mode=max): [Docker Docs: Cache modes](https://docs.docker.com/build/cache/backends/#cache-mode)



