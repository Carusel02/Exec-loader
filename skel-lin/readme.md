TEMA_SO MARIN_MARIUS_DANIEL 322_CC

-> IMPLEMENTARE

* programul primeste un executabil care genereaza page fault uri ce trebuiesc tratate
prin handler ul segv_handler ; partea de prindere a semnalelor  este deja rezolvata 
in skelet 

* la primirea unui semnal sig_segv, trebuie cautat segmentul corespunzator erorii ; am
folosit o functie care returneaza adresa segmentului gasit daca exista, sau NULL altfel ;
pentru a putea afla daca eroarea se afla intr un segment, se compara cu valoarea de start
si valoarea finala a segmentului respectiv, iterand printre toate segmentele executabilului

* se initializeaza apoi in variabile, datele segmentului (pentru usurinta si claritate cod),
apoi se creeaza o singura data, folosind variabila flag, zona custom ce reprezinta un vector
caracteristic de pagini pentru un segment, ulterior fiind linkat cu zona data a segmentului
corespunzator

* urmeaza maparea paginii care a cauzat fault-ul si aflarea indexului acesteia pentru calcule
ulterioare ; se foloseste functia mmap care primeste offsetul paginii pe care dorim sa o
alocam

* copierea datelor in paginile segmentelor tine cont de plasarea paginii in segment ; este 
necesar sa se afle in zona file_size , luand in considerare faptul ca alocarea se imparte
in 2 cazuri : cazul simplu (in care toata pagina poate fi incarcata si nu se depaseste 
file_size ul segmentului), cazul exceptional (in care se incarca doar o anumita parte
din dimensiunea unei pagini, pana la umplerea file_size ul) ; in cazul exceptional, se scade
din dimensiunea totala dimensiunea rezervata celorlalte pagini pentru a extrage lungimea 
dimensiunii de incarcare a bucatii

* dupa ce se copiaza datele, oferim paginii mapate drepturile segmentului din care face parte,
apoi o marcam in vectorul caracteristic din zona data pentru a evita o posibila mapare repetata
a acesteia

-> DURATA IMPLEMENTARE
3-4 zile ; cea mai lunga parte fiind documentatia necesara rezolvarii acestei teme

-> DETALII
detaliile legate de modul de lucru se gasesc in comentariile codului sursa