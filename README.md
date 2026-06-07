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
