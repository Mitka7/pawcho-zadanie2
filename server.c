#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

// Wbudowana funkcja pobierająca pogodę uwzględniająca MIASTO i KRAJ z zewnętrznego API (wttr.in)
void pobierz_pogode(const char* miasto, const char* kraj, char* wynik) {
    int sock;
    struct sockaddr_in serwer;
    struct hostent *he;
    char zadanie[512];
    char odpowiedz[4096] = {0};

    // Otwieranie gniazda sieciowego (socket)
    sock = socket(AF_INET, SOCK_STREAM, 0);
    // Zamiana adresu domenowego wttr.in na adres IP (zapytanie DNS)
    he = gethostbyname("wttr.in");
    if (he == NULL) { 
        strcpy(wynik, "Blad: Nie mozna znalezc adresu wttr.in (DNS)"); 
        close(sock); return; 
    }

    //Konfiguracja połączenia docelowego (port 80 to standardowy port HTTP)
    serwer.sin_family = AF_INET;
    serwer.sin_port = htons(80);
    serwer.sin_addr = *((struct in_addr **)he->h_addr_list)[0];

    //Łączenie się z serwerem wttr.in
    if (connect(sock, (struct sockaddr *)&serwer, sizeof(serwer)) < 0) {
        strcpy(wynik, "Blad: Brak polaczenia z internetem"); 
        close(sock); return;
    }

    // Budowanie surowego żądania HTTP. Udawanie przeglądarki.
    sprintf(zadanie, "GET /%s,%s?format=%%C+%%t HTTP/1.0\r\nHost: wttr.in\r\nUser-Agent: curl/7.68.0\r\nAccept-Language: pl\r\nConnection: close\r\n\r\n", miasto, kraj);
    write(sock, zadanie, strlen(zadanie));

    // Odczytanie odpowiedzi serwera w pętli
    int przeczytano;
    int suma = 0;
    while ((przeczytano = read(sock, odpowiedz + suma, sizeof(odpowiedz) - suma - 1)) > 0) {
        suma += przeczytano;
    }
    
    // Parsowanie odpowiedzi. Serwer zwraca nagłówki HTTP, a potem pustą linię (\r\n\r\n). Ważne jest to, co zanjduje się po pustej linii, czyli tekst z pogoda.
    char *cialo = strstr(odpowiedz, "\r\n\r\n");
    if (cialo) {
        cialo += 4; //Pominięcie znaku nowej linii
        int i = 0;
        //Kopiowanie tekstu, aż do natrafienia na koniec linii lub koniec stringa
        while(cialo[i] != '\n' && cialo[i] != '\r' && cialo[i] != '\0' && i < 127) {
            wynik[i] = cialo[i];
            i++;
        }
        wynik[i] = '\0'; //Zamknięnie stringa
    } else {
        strcpy(wynik, "Blad pobierania danych z serwera");
    }
    close(sock); //Zamknięcie połączenia
}

//Główna funkcja programu
int main(int argc, char *argv[]) {
    //Pobranie portu ze zmiennej środowiskowej lub ustawienie domyślnego 8080
    char *env_port = getenv("PORT");
    int port = (env_port != NULL) ? atoi(env_port) : 8080;

    // Mechanizm HEALTHCHECK
    if (argc > 1 && strcmp(argv[1], "health") == 0) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
        if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0) return 1; //Połączenie się nie udało
        close(sock);
        return 0; //Sukces - kontener jest zdrowy
    }

    // Logi startowe (Wymóg 1a)
    //Ustawienie strefy czasowej na polską
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    
    time_t t = time(NULL);
    printf("Data uruchomienia: %s", ctime(&t));
    printf("Autor: Weronika Mitaszka\n"); 
    printf("Nasluchuje na porcie TCP: %d\n", port);
    fflush(stdout); //Wypchnięcie tekstu do logów Dockera

    //Konfiguracja serwera WWW
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    //Główna pętla serwera 
    while (1) {
        int new_socket = accept(server_fd, NULL, NULL); //Oczekiwanie na wejście na stronę
        char buffer[2048] = {0};
        read(new_socket, buffer, 2048); //Czytanie zapytania od przeglądarki
        
        char response[2048]; //Bufor na odpowiedź HTML
        
        //ROUTING: Sprawdzenie czy to POST (wysłano formularz) czy GET (wejście na stronę)
        if (strstr(buffer, "POST") != NULL) {
            char city[64] = "Lublin";
            char country[64] = "Polska";

            // WYMÓG 1b: Pobieranie KRAJU z formularza
            char *country_ptr = strstr(buffer, "kraj=");
            if (country_ptr) {
                country_ptr += 5; //Przeskoczenie napisu "kraj="
                int i = 0;
                //Czekanie aż do napotkania znaku & lub spacji 
                while (country_ptr[i] != '&' && country_ptr[i] != ' ' && country_ptr[i] != '\r' && country_ptr[i] != '\n' && i < 63) {
                    country[i] = country_ptr[i];
                    i++;
                }
                country[i] = '\0';
            }

            // WYMÓG 1b: Pobieranie MIASTA z formularza
            char *city_ptr = strstr(buffer, "miasto=");
            if (city_ptr) {
                city_ptr += 7; //Przeskoczenie napisu "miasto="
                int i = 0;
                while (city_ptr[i] != '&' && city_ptr[i] != ' ' && city_ptr[i] != '\r' && city_ptr[i] != '\n' && i < 63) {
                    city[i] = city_ptr[i];
                    i++;
                }
                city[i] = '\0';
            }

            //Pobranie danych z API
            char pogoda_info[128];
            pobierz_pogode(city, country, pogoda_info);

            //Zwracanie wyniku do przeglądarki
            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
                              "<html><body style='text-align:center; font-family:sans-serif; padding:50px;'>"
                              "<h1>Pogoda na zywo</h1>"
                              "<h3>Kraj: %s | Miasto: %s</h3>"
                              "<p style='font-size: 24px; color: #333;'><strong>%s</strong></p>"
                              "<br><a href='/' style='padding: 10px 20px; background: #007bff; color: white; text-decoration: none; border-radius: 5px;'>Powrot do wyboru</a>"
                              "</body></html>", country, city, pogoda_info);
       } else {
            // Wysyłanie formularza (z wbudowanym skryptem JS sterującym listą miast)
            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
                              "<html><body style='text-align:center;font-family:sans-serif;padding:50px'>"
                              "<h2>Sprawdz pogode online</h2>"
                              "<form method='POST'>"
                              "<label>Kraj:</label>"
                              "<select name='kraj' id='k' onchange='u()' style='padding:5px;font-size:16px;margin-right:15px'>"
                              "<option value='Polska'>Polska</option><option value='Niemcy'>Niemcy</option>"
                              "<option value='UK'>Wielka Brytania</option><option value='Francja'>Francja</option></select>"
                              "<label>Miasto:</label><select name='miasto' id='m' style='padding:5px;font-size:16px'></select>"
                              "<br><br><button type='submit' style='padding:8px 20px;font-size:16px;background:#28a745;color:#fff;border:none;border-radius:5px'>Pobierz dane</button>"
                              "</form><script>const d={'Polska':['Lublin','Warszawa','Krakow'],'Niemcy':['Berlin','Monachium'],'UK':['Londyn','Edynburg'],'Francja':['Paryz','Lyon']};"
                              "function u(){let k=document.getElementById('k').value,m=document.getElementById('m');m.innerHTML='';d[k].forEach(c=>m.innerHTML+=\"<option value='\"+c+\"'>\"+c+\"</option>\")}u();"
                              "</script></body></html>");
        }
        //Wysłanie przygotowanego HTML do klienta
        write(new_socket, response, strlen(response));
        //Zamknięcie gniazda po wysłaniu odpowiedzi
        close(new_socket);
    }
    return 0;
}