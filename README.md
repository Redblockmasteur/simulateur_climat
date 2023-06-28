# simulateur_climat
Un projet réalisé à Adimaker comandé par POP Café.
En prévision d'ouverture d'un nouveau site d'enseignemnt à Creil, notre client souaite pouvoir proposer un démonstrateur de climat. L'objectif est de recréer les conditions météorologique d'une journée type renseigner par l'utilisateur. En associant plusieur serres ayant des paramètres différents, on peut montrer les effets du dérègelemnt climatique sur la flore qui se situe dans les serres. Une interface Web est prévue pour renseigner les consignes et propose de télécharger les données remonté par les capteurs au format CSV.

## Desciption technique
Le projet est animé par deux microcontroleurs :
Un ESP32 qui est charger d'éberger le site WEB utilisé pour renseinger les consignes, stoquer les données remontées par les cpateurs et de transmetrre les consignes à l'Arduino.

Un Arduino Uno, qui est connecté aux capteurs et qui recois les consignes fixé par l'utilisateur et se charge de les maintenir. Pour y parvenir l'Arduino dispose d'une résitance chauffante, d'un servo-moteur qui permet d'ouvrir le toit pour réduire l'humidité et une pompe pour iriger les plantes.


### Montage électronique :
Sur l'image ci-dessous vous trouvrez un shémat du montage ellectronique.
![MicrosoftTeams-image](https://github.com/Redblockmasteur/simulateur_climat/assets/46867831/88ec1d7d-ad6e-41fc-8c16-d727eac57270)

### Limitations connues
Le code hébergé dans ce repository assume que les capteurs suivant sont présent:  
Un capteur de température et d'humidité de l'Air "DHT11"
Un capteur d'humiditée du sol Analogique
Un servomoteur 5V controlée par PWM


## Mise en place du projet 
1. Télécharger se repository
2. Télécharger https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/ (Compatible arduino 1.8.X uniquement)
3. Configurer le code de l'ESP -> voir wiki
4. Flasher le code et les données sur l'ESP32
5. Flasher le code sur l'Arduino
6. Connecter les ESP32 et l'Arduino et les capteur conformément au shéma.
