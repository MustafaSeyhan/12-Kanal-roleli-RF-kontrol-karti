/*
PROGRAMIN GENEL ÇALIÞMA MANTIÐI;
 * Bu program 12 kanallý elcik kumanda adýyla geçen kumandaya uygun yazýldý.
 
 * her kumandanýn þifre bilgisini lojik analizör veya dijital osiloskop ile okuyup programa entegre etmek gerekiyor. Çünkü kumandalarýn dýþ
 görünüþü ayn olsada içinde kullanýlan entegreler ve haliyle veri paketlerinin yapýlarý deðiþebiliyor.

 * 433mhz alýcýnýn DO çýkýþý mcu'nun int_ext giriþine baðlý. her tuþa basmada gelen verinin yükselen kenarýnda kesme oluþup veri okunuyor. 
Dolayýsýyla sürekli okuma yapan bir sistem.

 * gelen veri paketinin ilk 16 bitlik kýsmý kumandanýn þifre bilgisi, sonraki 8 bitlik veri tuþun numarasý olarak geliyor. 
25. bit(senkron biti) daima 0 olduðu için programda göz ardý ediliyor.

* SC2262 entegresi 8 adet adres bitine sahip. Adres biti "0" (L) konumunda olunca þifre 00, "1" (H) konumunda olunca þifre 11 
ve adres biti boþta (F) olunca 01 bilgisi ile þifre üretiyor. Yani her adres bitinin 3 farklý deðeri var.

 * haberleþme sinyalinde her bir bit yaklaþýk 2000usn sürüyor. veri paketlerinin arasýndaki boþluk dahil bir veri paketinin süresi 
 yaklaþýk ~55msn sürüyor. bu bilgi tüm röleleri kapatma kontrolü için tuþa sürekli basýldýðýnýn kontrol edildiði kýsýmda kullanýlýyor.

 * timer0 kesmesi 32.7msn olarak ayarlandý.
 
 * kumandanýn yolladýðý her bitin süresi 2000usn ve lojik 1 olanlar geniþ, 0 olanlar dar sinyal süresine sahip. 
 lojik 1 için ~1500usn high + ~500usn low süresi kullanýlýrken
 lojik 0 için ~500usn high + ~1500usn low süresi kullanýlýyor. 
 
 * yukarýdaki açýklamaya istinaden her gelen verinin yükselen kenarýndan 1000usn sonra veri kontrol ediliyor. eðer gelen veri "0" ise 
bu anda okunan sinyal de "0" oluyor. deðilse "1" oluyor.

 * her gelen yükselen kenardan sonra veri okunuyor, gerekli kaydýrma iþlemlerinden sonra o ana kadar oluþan veri deðeri röle kontrol
 kýsmýnda kontrol edilerek gelen veri uygunsa gerekli iþlem yapýlýyor.
 
 * her röle iþleminden sonra timer0 sayýcýsý sýfýrlanýyor. Bu sayede kullanýcýnýn ne kadar süre boyunca butona basýlý tuttuðu kontrol ediliyor.
 
 * röle toggle iþleminden sonra kullanýcýnýn halen kumandaya basýp basmadýðý, timer0 deðerinin bir veri paketinden(45msn) daha uzun süreli
 bir sayaç deðerine ulaþýp ulaþmadýðýndan anlaþýlýyor.
 
 * diyelim ki 5 röle aktif durumda. kullanýcý 8. röleye bastý, röle toggle yaptý. 500msn geçti sonra timer sayacý sýfýrlandý.
 sayac her 32.7msn de bir deðer arttýrýyor. bu arada gelen veriler halen okunuyor. eðer kullanýcý elini ayný tuþa basmaya devam ediyorsa
 gelen veri önceki tuþ olan 8 numara ile eþleþti ise ve timer sayacý <=2 ise yani geçen süre 65msn den az ise kullanýcý elini butondan 
 çekmemiþ demektir. bu sayede tüm röleler pasif duruma getirilebilir.

 * bu program SC2262 entegresine göre yazýldý. ilk dört bayt adres verisi olutor. Sonra gelen 8 bitlik veri tuþun numarasý yerine geçiyor. 
 25. bit senkron biti olduðundan göz ardý ediliyor.
 veride kayýp olmasý durumunu engellemek için entegre basýlan tuþ numarasýndaki her bir biti iki bit ile ifade ediyor. 
 ÖRNEK : diyelim ki 3 numaralý tuþa basýlmýþ olsun. 3'ün binary karþýlýðý 0x0011 iken her sýfýr için iki defa sýfýr ve 
 her bir için iki defa 1 kullanýlmýþ. yani 3 tuþunun karþýlýðý  0x00001111 yani 0F olmuþ. bu deðerin baþýna bu kumanda için ayarlanan 
 0x545F kodu da eklenince 3 tuþunun karþýlýðý tam olarak 0x545F0F oluyor. (25. biti görmezden geliyoruz)
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
volatile unsigned int8     guvenlik_icin_veri_sifirlama_sayaci=0;// devre boþta çalýþýrken arada sýrada kendi kendine bazý röleleri açýyordu. güvenliði arttýrmak adýna
                           // devrede kumandadan veri giriþi yokken yaklaþýk her 1 saniyede bir defa gelen veri kayýtçýsýný temizlemek için bu kayýtçýyý timer kesmesinde
                           // arttýrýp kontrol elemaný olarak kullanýyorum.

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
   guvenlik_icin_veri_sifirlama_sayaci++;// devre boþta dururken bir kaç defa kendi kendine röleyi açtý. bunu engellemek için tuþa basýlý deðilken
   if(guvenlik_icin_veri_sifirlama_sayaci >=10 )// yaklaþýk her 1 saniyede 1 defa gelen veri bilgileri silinecek, bir de böyle deneyeyim.
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
   kesme_geldimi=hayir; // kesme oluþtuðunda int_ext fonksiyonunda bu bilgi "evet" olarak yazýlýr. 
   while(kesme_geldimi==hayir)
   {}   
   gelen_veri=gelen_veri<<1;  // önceki veri bir bit sola ötelenip aþaðýda yeni veri lsb bitine yazýlýr.
   delay_us(1000); // eðer gelen veri 0 ise sinyalin yükselen kenarýndan 1000usn sonra 0 konumuna düþmüþ oluyor. 
   if(input(rf_data)==1)   // eðer bu noktada rf_data=0 ise gelen veri "0",  rf_data=0 ise gelen veri "1" demektir.
   {
   bit_set(gelen_veri,0);// gelen veri "1" imiþ.
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
   veri = gelen_veri & 0x00FFFFFF; // orjinal gelen_veri üzerinde oynama yapmamak için bu eþitleme yapýldý.

   if(timer_sayaci<=2) // en son basýlan tuþtan 500msn süre geçtikten ve bu sürenin hemen ardýndan(~60msn içinde) tuþa basýldýysa bu durum 
               {     // kullanýcýnýn halen butona bastýðý olarak yorumlanýr.
               
               if( veri ==  onceki_veri ) // deðerlendirmeye alýnan veri önceki veri ile aynýysa tüm rölelerin kapatma iþlemi yapýlýr. 
                   { // önceki veri ile ayný olma þartýnýn sebebi þu; kullanýcý tuþlara hýzlý hýzlý sýrasý ile basýnca yukarýda bahsedilen
                   // kurgudaki 60msn içinde yeniden basýlan yani halen basýlmaya devam edilen buton algýsýný oluþturuyordu. Bunu engellemek
                   // için kullanýcý ne kadar sýk aralýklarla basarsa bassýn þimdiki basýlan tuþ önceki ile ayný deðilse yani kullancý
                   // tüm butonlarý sýrasýyla geziyorsa tüm rölelerin kapatýlmasý iþlemi yapýlmayacak demektir.
                        output_low(role_1);   output_low(role_2);   output_low(role_3);   output_low(role_4);
                        output_low(role_5);   output_low(role_6);   output_low(role_7);   output_low(role_8);
                        output_low(role_9);   output_low(role_10);   output_low(role_11);   output_low(role_12);
                        timer_sayaci=50;
                        for(genel_sayac=40;genel_sayac>=1;genel_sayac--)
                        {
                        output_toggle(led_kullanici); // kullanýcýya geri bildirim vermek için yazýlan kod.
                        delay_ms(50);
                        }
                   }               
               }
   
   /*
   aþaðýdaki kodda kullanýlan 0x545F kodu kumandanýn kendisine ait olan 4 karakterlik kodudur(þifresidir). bu kod lojik analizör verileri
   incelenerek bulundu. bu devrenin dökümantasyonunda mevcut. 0x545F kodundan gelen sonraki  da basýlan tuþun numarasýný ifade ediyor.
   */
   
   else if(timer_sayaci>2)// daha önce tuþa basýlmýþ, röle çektikten ve 500msn bekledikten sonra sayaç sýfýrlanmýþ.
   {                 //önceki veriden sonra yaklaþýk 70msn zaman geçmiþ.
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




