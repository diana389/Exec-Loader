# Loader de Executabile

Handler-ul implementat incepe prin a verifica din ce segment al executabilului face parte adresa la care s-a produs page fault-ul. Indexul acestuia este returnat de functia `find_seg`. In cazul in care adresa nu se gaseste in niciun segment, înseamnă că este un **acces invalid** la memorie si se rulează handler-ul default de page fault.

In continuare, se afla indexul paginii care incadreaza adresa page fault-ului prin apelul functiei `find_page`. In cazul in care pagina este deja mapata (lucru verificat prin vectorul de frecventa asociat), se încearcă un **acces la memorie nepermis** (segmentul respectiv nu are permisiunile necesare) si  se rulează handler-ul default de page fault. Altfel, pagina se mapeaza si se marcheaza acest lucru in vectorul de frecventa salvat in `data`.

Se repozitioneaza offset-ul fisierului la inceputul paginii gasite si se citeste din fisier.

La final, se seteaza permisiunile segmentului curent.
