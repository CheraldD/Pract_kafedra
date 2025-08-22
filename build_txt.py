import os

# --- Конфигурация ---
SOURCE_DIRECTORY = 'src'                 # Папка с исходным кодом
OUTPUT_FILENAME = 'project_listing.txt'  # Имя итогового текстового файла для листинга

def find_source_files(directory):
    """
    Рекурсивно находит все файлы .h и .cpp в указанной директории.
    Возвращает отсортированный список путей к файлам.
    """
    found_files = []
    print(f"Сканирование директории '{directory}'...")
    
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(('.h', '.cpp')):
                full_path = os.path.join(root, file)
                found_files.append(full_path)
    
    found_files.sort()
    print(f"Найдено {len(found_files)} файлов с исходным кодом.")
    return found_files

def write_listing_with_page_breaks(files, output_path):
    """
    Собирает содержимое всех найденных файлов в один текстовый файл,
    разделяя каждый файл управляющим символом разрыва страницы ('\f').
    """
    print(f"Создание файла листинга '{output_path}'...")
    
    try:
        with open(output_path, 'w', encoding='utf-8') as listing_file:
            for i, file_path in enumerate(files):
                # Нормализуем путь для единообразного вида (используем '/')
                normalized_path = file_path.replace('\\', '/')
                
                print(f" > Обработка файла: {normalized_path}")
                
                # --- 1. Название модуля ---
                listing_file.write(f"Файл: {normalized_path}\n")
                listing_file.write("-" * (len(normalized_path) + 6) + "\n\n")
                
                # --- 2. Код ---
                try:
                    # Читаем содержимое исходного файла в кодировке UTF-8
                    with open(file_path, 'r', encoding='utf-8') as src_file:
                        content = src_file.read()
                        listing_file.write(content)
                except UnicodeDecodeError:
                    # Резервный вариант
                    with open(file_path, 'r', encoding='latin-1') as src_file:
                        content = src_file.read()
                        listing_file.write(content)
                
                # --- 3. Разрыв страницы ---
                # Добавляем разрыв после каждого файла, кроме самого последнего
                if i < len(files) - 1:
                    # Вставляем символ "Form Feed".
                    # При копировании в LibreOffice Writer это создаст разрыв страницы.
                    listing_file.write('\f')

        print("\nСборка листинга успешно завершена!")
        print(f"Результат сохранен в файл: {os.path.abspath(output_path)}")

    except IOError as e:
        print(f"\nОшибка! Не удалось записать в файл: {e}")
    except Exception as e:
        print(f"\nПроизошла непредвиденная ошибка: {e}")

def main():
    """
    Главная функция скрипта.
    """
    if not os.path.isdir(SOURCE_DIRECTORY):
        print(f"Ошибка: Директория '{SOURCE_DIRECTORY}' не найдена.")
        print("Пожалуйста, запустите скрипт из корневой директории вашего проекта.")
        return

    source_files = find_source_files(SOURCE_DIRECTORY)
    
    if not source_files:
        print(f"В директории '{SOURCE_DIRECTORY}' не найдено ни одного файла .h или .cpp.")
        return
        
    write_listing_with_page_breaks(source_files, OUTPUT_FILENAME)

if __name__ == "__main__":
    main()