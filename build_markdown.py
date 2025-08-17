import os
import datetime

# --- Конфигурация ---
SOURCE_DIRECTORY = 'src'       # Папка с исходным кодом
OUTPUT_FILENAME = 'project_source.md' # Имя конечного Markdown файла
PROJECT_TITLE = 'Полный Исходный Код Проекта Системы Управления Доступом'

# --- Основная логика ---

def find_source_files(directory):
    """
    Рекурсивно находит все файлы .h и .cpp в указанной директории.
    Возвращает отсортированный список путей к файлам.
    """
    found_files = []
    print(f"Сканирование директории '{directory}'...")
    
    # os.walk рекурсивно обходит дерево каталогов
    for root, _, files in os.walk(directory):
        for file in files:
            # Проверяем, что файл имеет нужное расширение
            if file.endswith(('.h', '.cpp')):
                # Сохраняем полный путь к файлу
                full_path = os.path.join(root, file)
                found_files.append(full_path)
    
    found_files.sort() # Сортируем для предсказуемого порядка в .md файле
    print(f"Найдено {len(found_files)} файлов с исходным кодом.")
    return found_files

def write_markdown_file(files, output_path, title):
    """
    Собирает содержимое всех найденных файлов в один Markdown файл.
    """
    print(f"Создание файла '{output_path}'...")
    try:
        with open(output_path, 'w', encoding='utf-8') as md_file:
            # --- Заголовок документа ---
            md_file.write(f"# {title}\n")
            md_file.write(f"_Сгенерировано: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}_\n\n")
            md_file.write("## Содержание\n")
            
            # --- Создаем оглавление ---
            for file_path in files:
                # Нормализуем путь для Markdown ссылок
                normalized_path = file_path.replace('\\', '/')
                anchor = normalized_path.replace('/', '').replace('.', '')
                md_file.write(f"- [`{normalized_path}`](#{anchor})\n")
            md_file.write("\n---\n\n")

            # --- Добавляем содержимое каждого файла ---
            for file_path in files:
                normalized_path = file_path.replace('\\', '/')
                anchor = normalized_path.replace('/', '').replace('.', '')
                
                print(f" > Добавление файла: {normalized_path}")
                
                md_file.write(f"## <a name=\"{anchor}\"></a>Файл: `{normalized_path}`\n\n")
                md_file.write("```cpp\n")
                
                try:
                    # Читаем содержимое исходного файла
                    with open(file_path, 'r', encoding='utf-8') as src_file:
                        content = src_file.read()
                        md_file.write(content)
                except UnicodeDecodeError:
                    # Резервный вариант на случай, если какой-то файл в другой кодировке
                    with open(file_path, 'r', encoding='latin-1') as src_file:
                        content = src_file.read()
                        md_file.write(content)
                        
                md_file.write("\n```\n\n")
                md_file.write("---\n\n")

        print("\nСборка успешно завершена!")
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
        print("В директории 'src' не найдено ни одного файла .h или .cpp.")
        return
        
    write_markdown_file(source_files, OUTPUT_FILENAME, PROJECT_TITLE)

# Точка входа в программу
if __name__ == "__main__":
    main()