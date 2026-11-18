# Regeln für Claude (Bandampel)

Status: Nur die Datei `STATUS.md` im Hauptordner dieses Repositorys (Bandampel) lesen, ohne Rückfrage, höchstens 30 Zeilen. Keine STATUS.md aus anderen Repositories. Auf Wunsch von Marcel Sauder vor Chat-Ende überschreiben, im selben PR wie die Arbeit.

Quelle der Wahrheit ist dieses Repository auf GitHub, nicht ein lokaler Stand und nicht das Claude Projekt.

1. Nie auf `main` pushen, kein Force Push, keinen Branch eines anderen Chats pushen. Branch und Pull Request, 1 Thema pro PR. Einzige Ausnahme war der Initial-Commit.
2. Vor dem Start `git fetch` und von `origin/main` abzweigen. Vor jedem Push `origin/main` mergen, kein Rebase. Gemergter Branch ist erledigt. Claude merged PRs nicht selbst.
3. Konflikte in Dateien anderer Chats melden, nicht lösen.
4. Nie löschen oder umbenennen ohne Auftrag von Marcel Sauder. Keine Zugangsdaten (WLAN, API-Schlüssel) und keine Personendaten im Repository.
5. Stil: Deutsch (englische Zusammenfassung im README), Umlaute, «ss», keine Gedankenstriche, Ziffern, knapp, keine Microsoft-Formate.
6. Nichts als entschieden eintragen: «offen» oder «Vorschlag».
7. Kosten: Vor Modellwechsel, Subagenten, grossen Lesezugriffen, Websuchen oder anderem, was mehr Traffic verursacht, Marcel Sauder fragen und auf Mehrkosten hinweisen.
8. Commits und Pull Requests: keine Claude-Zeilen (kein Co-Authored-By, kein Claude-Session, kein Generated with). Autor und Committer sind Marcel Sauder (anonyme GitHub-Adresse). Copyright: `FerienBSD aka Marcel Sauder`.
9. Rückfragen an Marcel Sauder immer nummeriert (1., 2., 3.), damit er mit der Nummer antworten kann.
10. Antworten ohne Nebeninfos, mit denen Marcel Sauder nichts anfangen kann (Prüfsummen, Dateigrössen, Hashes). Nur zeigen, was er braucht oder tun kann.
11. Lizenzen: Firmware GPL-3.0-only, Hardware CERN-OHL-P-2.0, Doku CC-BY-4.0. Neue Dateien tragen den passenden SPDX-Hinweis.
12. Datenquellen (HamQSL, NOAA) schonend abfragen: höchstens alle 30 Minuten, Quelle im README nennen.
13. Versionen: Jeder gemergte PR bekommt eine Version 1.x.y (neue Funktion: x erhöhen, Fehlerbehebung oder Doku: y erhöhen). `FW_VERSION` in `bandampel.ino` und `CHANGELOG.md` im selben PR nachführen. Tag und Release `v1.x.y` legt Marcel Sauder nach dem Merge an (Claude darf das nicht). `tools/releases_anlegen.sh` enthält die Befehle für 1.0.0 bis 1.17.2.
