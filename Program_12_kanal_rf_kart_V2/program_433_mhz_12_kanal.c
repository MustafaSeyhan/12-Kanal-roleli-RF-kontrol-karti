/*
PROGRAMIN GENEL �ALI�MA MANTI�I;
 * Bu program 12 kanall� elcik kumanda ad�yla ge�en kumandaya uygun yaz�ld�.
 
 * her kumandan�n �ifre bilgisini lojik analiz�r veya dijital osiloskop ile okuyup programa entegre etmek gerekiyor. ��nk� kumandalar�n d��
 g�r�n��� ayn olsada i�inde kullan�lan entegreler ve haliyle veri paketlerinin yap�lar� de�i�ebiliyor.

 * 433mhz al�c�n�n DO ��k��� mcu'nun int_ext giri�ine ba�l�. her tu�a basmada gelen verinin y�kselen kenar�nda kesme olu�up veri okunuyor. 
Dolay�s�yla s�rekli okuma yapan bir sistem.

 * gelen veri paketinin ilk 16 bitlik k�sm� kumandan�n �ifre bilgisi, sonraki 8 bitlik veri tu�un numaras� olarak geliyor. 
25. bit(senkron biti) daima 0 oldu�u i�in programda g�z ard� ediliyor.

* SC2262 entegresi 8 adet adres bitine sahip. Adres biti "0" (L) konumunda olunca �ifre 00, "1" (H) konumunda olunca �ifre 11 
ve adres biti bo�ta (F) olunca 01 bilgisi ile �ifre �retiyor. Yani her adres bitinin 3 farkl� de�eri var.

 * haberle�me sinyalinde her bir bit yakla��k 2000usn s�r�yor. veri paketlerinin aras�ndaki bo�luk dahil bir veri paketinin s�resi 
 yakla��k ~55msn s�r�yor. bu bilgi t�m r�leleri kapatma kontrol� i�in tu�a s�rekli bas�ld���n�n kontrol edildi�i k�s�mda kullan�l�yor.

 * timer0 kesmesi 32.7msn olarak ayarland�.
 
 * kumandan�n yollad��� her bitin s�resi 2000usn ve lojik 1 olanlar geni�, 0 olanlar dar sinyal s�resine sahip. 
 lojik 1 i�in ~1500usn high + ~500usn low s�resi kullan�l�rken
 lojik 0 i�in ~500usn high + ~1500usn low s�resi kullan�l�yor. 
 
 * yukar�daki a��klamaya istinaden her gelen verinin y�kselen kenar�ndan 1000usn sonra veri kontrol ediliyor. e�er gelen veri "0" ise 
bu anda okunan sinyal de "0" oluyor. de�ilse "1" oluyor.

 * her gelen y�kselen kenardan sonra veri okunuyor, gerekli kayd�rma i�lemlerinden sonra o ana kadar olu�an veri de�eri r�le kontrol
 k�sm�nda kontrol edilerek gelen veri uygunsa gerekli i�lem yap�l�yor.
 
 * her r�le i�leminden sonra timer0 say�c�s� s�f�rlan�yor. Bu sayede kullan�c�n�n ne kadar s�re boyunca butona bas�l� tuttu�u kontrol ediliyor.
 
 * r�le toggle i�leminden sonra kullan�c�n�n halen kumandaya bas�p basmad���, timer0 de�erinin bir veri paketinden(45msn) daha uzun s�reli
 bir saya� de�erine ula��p ula�mad���ndan anla��l�yor.
 
 * diyelim ki 5 r�le aktif durumda. kullan�c� 8. r�leye bast�, r�le toggle yapt�. 500msn ge�ti sonra timer sayac� s�f�rland�.
 sayac her 32.7msn de bir de�er artt�r�yor. bu arada gelen veriler halen okunuyor. e�er kullan�c� elini ayn� tu�a basmaya devam ediyorsa
 gelen veri �nceki tu� olan 8 numara ile e�le�ti ise ve timer sayac� <=2 ise yani ge�en s�re 65msn den az ise kullan�c� elini butondan 
 �ekmemi� demektir. bu sayede t�m r�leler pasif duruma getirilebilir.

 * bu program SC2262 entegresine g�re yaz�ld�. ilk d�rt bayt adres verisi olutor. Sonra gelen 8 bitlik veri tu�un numaras� yerine ge�iyor. 
 25. bit senkron biti oldu�undan g�z ard� ediliyor.
 veride kay�p olmas� durumunu engellemek i�in entegre bas�lan tu� numaras�ndaki her bir biti iki bit ile ifade ediyor. 
 �RNEK : diyelim ki 3 numaral� tu�a bas�lm�� olsun. 3'�n binary kar��l��� 0x0011 iken her s�f�r i�in iki defa s�f�r ve 
 her bir i�in iki defa 1 kullan�lm��. yani 3 tu�unun kar��l���  0x00001111 yani 0F olmu�. bu de�erin ba��na bu kumanda i�in ayarlanan 
 0x545F kodu da eklenince 3 tu�unun kar��l��� tam olarak 0x545F0F oluyor. (25. biti g�rmezden geliyoruz)
*/
#include <16F876.h>
#device ADC=10

#FUSES NOBROWNOUT                 //Reset when brownout detected
#FUSES NOPROTECT                  //Code protected from reads
#FUSES NOCPD                      //Data EEPROM Code Protected
#FUSES NOWDT                    //No Watch Dog Timer
#FUSES PUT                      //Power Up Timer

#priority EXT, RTCC

#use delay(crystal=4000000)
#use fast_io(A)
#use fast_io(B)
#use fast_io(C)

#define mod_sec_1          PIN_A0    
#define mod_sec_2          PIN_A1
#define mod_sec_3          PIN_A2    
#define mod_sec_4          PIN_A3   
#define rf_data            PIN_B0
#define buton_kullanici    PIN_C2

#define role_1             PIN_C3
#define role_2             PIN_C4
#define role_3             PIN_C5
#define role_4             PIN_C6
#define role_5             PIN_C7
#define role_6             PIN_B1
#define role_7             PIN_B2
#define role_8             PIN_B3
#define role_9             PIN_B4
#define role_10            PIN_B5
#define role_11            PIN_B6
#define role_12            PIN_B7
#define led_kullanici      PIN_C1

#define evet                  5
#define hayir                 10
#define role_genel_kapatma_yapildi  5
#define role_genel_kapatma_yok      10

volatile unsigned int8     kesme_geldimi=hayir;
volatile unsigned int16    genel_sayac=0;
volatile unsigned int8     veri_geldimi=0;
volatile unsigned int32    gelen_veri=0;
volatile unsigned int32    veri=0;
volatile unsigned int32    onceki_veri=0;
volatile unsigned int32    timer_sayaci=0;
volatile unsigned int8     guvenlik_icin_veri_sifirlama_sayaci=0;// devre bo�ta �al���rken arada s�rada kendi kendine baz� r�leleri a��yordu. g�venli�i artt�rmak ad�na
                           // devrede kumandadan veri giri�i yokken yakla��k her 1 saniyede bir defa gelen veri kay�t��s�n� temizlemek i�in bu kay�t��y� timer kesmesinde
                           // artt�r�p kontrol eleman� olarak kullan�yorum.

//*************************************************************************************************
//*************************************************************************************************
void sistem_test();
void mcu_init();
void ram_init();
void role_ayarla();

//*************************************************************************************************
//*************************************************************************************************
#INT_RTCC
void  RTCC_isr(void) 
{
   guvenlik_icin_veri_sifirlama_sayaci++;// devre bo�ta dururken bir ka� defa kendi kendine r�leyi a�t�. bunu engellemek i�in tu�a bas�l� de�ilken
   if(guvenlik_icin_veri_sifirlama_sayaci >=10 )// yakla��k her 1 saniyede 1 defa gelen veri bilgileri silinecek, bir de b�yle deneyeyim.
   {
   guvenlik_icin_veri_sifirlama_sayaci=0;
   gelen_veri=0;
   }   
      
   if(timer_sayaci<50)  
   {
   timer_sayaci++;
   }
}
//*************************************************************************************************
//*************************************************************************************************
#INT_EXT
void  EXT_isr(void) 
{
   kesme_geldimi=evet;
}
//*************************************************************************************************
//*************************************************************************************************
void veri_al()
{
   enable_interrupts(INT_EXT);
   kesme_geldimi=hayir; // kesme olu�tu�unda int_ext fonksiyonunda bu bilgi "evet" olarak yaz�l�r. 
   while(kesme_geldimi==hayir)
   {}   
   gelen_veri=gelen_veri<<1;  // �nceki veri bir bit sola �telenip a�a��da yeni veri lsb bitine yaz�l�r.
   delay_us(1000); // e�er gelen veri 0 ise sinyalin y�kselen kenar�ndan 1000usn sonra 0 konumuna d��m�� oluyor. 
   if(input(rf_data)==1)   // e�er bu noktada rf_data=0 ise gelen veri "0",  rf_data=0 ise gelen veri "1" demektir.
   {
   bit_set(gelen_veri,0);// gelen veri "1" imi�.
   }   
}
//*************************************************************************************************
//*************************************************************************************************
void role_aktiflestirme_islem_sonu()
{
   disable_interrupts(INT_EXT);
   delay_ms(500); 
   timer_sayaci=0; 
   output_low(led_kullanici);
   enable_interrupts(INT_EXT);
}
//*************************************************************************************************
//*************************************************************************************************
void main()
{
   mcu_init();
   ram_init();
   sistem_test();
  
   while(true)
   {   
   veri_al();
   role_ayarla();
   }
}

//*************************************************************************************************
//*************************************************************************************************
void mcu_init()
{
   setup_timer_0(RTCC_INTERNAL|RTCC_DIV_128|RTCC_8_BIT);      //32.765 msn overflow
   enable_interrupts(INT_RTCC);   
   enable_interrupts(INT_EXT);
   EXT_INT_EDGE(L_TO_H);
   enable_interrupts(GLOBAL);
   

   set_tris_a(0b11111111);
   set_tris_b(0b00000001);
   set_tris_c(0b00000101);
   port_B_pullups(FALSE);
   output_a(0x00);   
   output_b(0x00);
   output_c(0x00);
   }
//*************************************************************************************************
//*************************************************************************************************
void ram_init()
{
      genel_sayac=0;
      veri_geldimi=hayir;
      gelen_veri=0;
      kesme_geldimi=hayir;
      timer_sayaci=50;
      onceki_veri=0;
      guvenlik_icin_veri_sifirlama_sayaci=0;
}  
//*************************************************************************************************
//*************************************************************************************************
void role_ayarla()
{
   veri = gelen_veri & 0x00FFFFFF; // orjinal gelen_veri �zerinde oynama yapmamak i�in bu e�itleme yap�ld�.

   if(timer_sayaci<=2) // en son bas�lan tu�tan 500msn s�re ge�tikten ve bu s�renin hemen ard�ndan(~60msn i�inde) tu�a bas�ld�ysa bu durum 
               {     // kullan�c�n�n halen butona bast��� olarak yorumlan�r.
               
               if( veri ==  onceki_veri ) // de�erlendirmeye al�nan veri �nceki veri ile ayn�ysa t�m r�lelerin kapatma i�lemi yap�l�r. 
                   { // �nceki veri ile ayn� olma �art�n�n sebebi �u; kullan�c� tu�lara h�zl� h�zl� s�ras� ile bas�nca yukar�da bahsedilen
                   // kurgudaki 60msn i�inde yeniden bas�lan yani halen bas�lmaya devam edilen buton alg�s�n� olu�turuyordu. Bunu engellemek
                   // i�in kullan�c� ne kadar s�k aral�klarla basarsa bass�n �imdiki bas�lan tu� �nceki ile ayn� de�ilse yani kullanc�
                   // t�m butonlar� s�ras�yla geziyorsa t�m r�lelerin kapat�lmas� i�lemi yap�lmayacak demektir.
                        output_low(role_1);   output_low(role_2);   output_low(role_3);   output_low(role_4);
                        output_low(role_5);   output_low(role_6);   output_low(role_7);   output_low(role_8);
                        output_low(role_9);   output_low(role_10);   output_low(role_11);   output_low(role_12);
                        timer_sayaci=50;
                        for(genel_sayac=40;genel_sayac>=1;genel_sayac--)
                        {
                        output_toggle(led_kullanici); // kullan�c�ya geri bildirim vermek i�in yaz�lan kod.
                        delay_ms(50);
                        }
                   }               
               }
   
   /*
   a�a��daki kodda kullan�lan 0x545F kodu kumandan�n kendisine ait olan 4 karakterlik kodudur(�ifresidir). bu kod lojik analiz�r verileri
   incelenerek bulundu. bu devrenin d�k�mantasyonunda mevcut. 0x545F kodundan gelen sonraki  da bas�lan tu�un numaras�n� ifade ediyor.
   */
   
   else if(timer_sayaci>2)// daha �nce tu�a bas�lm��, r�le �ektikten ve 500msn bekledikten sonra saya� s�f�rlanm��.
   {                 //�nceki veriden sonra yakla��k 70msn zaman ge�mi�.
        if(veri==(0x545f03) ){ onceki_veri=veri; output_high(led_kullanici); output_toggle(role_1)  ;role_aktiflestirme_islem_sonu();}
   else if(veri==(0x545f0C) ){ onceki_veri=veri; output_high(led_kullanici); output_toggle(role_2)  ;role_aktiflestirme_islem_sonu();} 
   else if(veri==(0x545f0F) ){ onceki_veri=veri; output_high(led_kullanici); output_toggle(role_3)  ;role_aktiflestirme_islem_sonu();} 
   else if(veri==(0x545f30) ){ onceki_veri=veri; output_high(led_kullanici); output_toggle(role_4)  ;role_aktiflestirme_islem_sonu();} 
   else if(veri==(0x545f33) ){ onceki_veri=veri; output_high(led_kullanici); output_toggle(role_5)  ;role_aktiflestirme_islem_sonu();} 
   else if(veri==(0x545f3C) ){ onceki_veri=veri; output_high(led_kullanici); output_toggle(role_6)  ;role_aktiflestirme_islem_sonu();}
   else if(veri==(0x545f3F) ){ onceki_veri=veri; output_high(led_kullanici); output_toggle(role_7)  ;role_aktiflestirme_islem_sonu();} 
   else if(veri==(0x545fC0) ){ onceki_veri=veri; output_high(led_kullanici); output_toggle(role_8)  ;role_aktiflestirme_islem_sonu();} 
   else if(veri==(0x545fC3) ){ onceki_veri=veri; output_high(led_kullanici); output_toggle(role_9)  ;role_aktiflestirme_islem_sonu();} 
   else if(veri==(0x545fCC) ){ onceki_veri=veri; output_high(led_kullanici); output_toggle(role_10) ;role_aktiflestirme_islem_sonu();}  
   else if(veri==(0x545fCF) ){ onceki_veri=veri; output_high(led_kullanici); output_toggle(role_11) ;role_aktiflestirme_islem_sonu();}  
   else if(veri==(0x545fF0) ){ onceki_veri=veri; output_high(led_kullanici); output_toggle(role_12) ;role_aktiflestirme_islem_sonu();}   
   }
    
}


//*************************************************************************************************
//*************************************************************************************************
void sistem_test(){   
   output_low(led_kullanici);   
   for(genel_sayac=8;genel_sayac>=1;genel_sayac--)
   {
   output_toggle(led_kullanici);
   output_low(role_1);   output_low(role_2);   output_low(role_3);   output_low(role_4);
   output_low(role_5);   output_low(role_6);   output_low(role_7);   output_low(role_8);
   output_low(role_9);   output_low(role_10);  output_low(role_11);  output_low(role_12);
   delay_ms(125);
   }
   output_low(led_kullanici);
}




