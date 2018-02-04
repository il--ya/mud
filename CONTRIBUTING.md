# Как внести свой вклад в развитие исходного кода Былин
## Правила написания кода

В этом разделе описаны некоторые правила и рекомендации которым нужно следовать
при внесении своей лепты в код Былин.

При написании кода вы должны следовать правилу "одна команда - одна строка".
Желательно это же правило распространять и на объявления переменных.

Былины используют стандарт C++17.

Тела конструкций if, if ... else for, while, do ... while всегда должны быть
заключены в символы '{', '}'.

Функции должны предпочитаться макросам. Вообще, следует стараться избегать
использования макросов и избавляться от них, где это возможно.

Если тип данных используется более одного раза с одинаковой семантикой, то для
него следует ввести псевдоним и в дальнейшем использовать только его.

Например:

    using meat_mapping_t = std::pair<obj_vnum, obj_vnum>;

Код должен быть компилируемым на Windows, Linux. При компиляции вы должны
уделять особое внимание предупреждениям компилятора и исправлять их при первой
же возможности. Особенно если предупреждения появились вследствие добавленного
вами кода.

Старайтесь перед коммитом своих изменений запускать unit-тесты. Старайтесь
писать свои unit-тесты для написанного вами кода. Часто ошибки можно обнаружить
ещё до того, как они станут проблемой.

### Отступы

Все исходные коды должны иметь символ табуляции в качестве символа отступа.

Не допускается использование двух и более подряд идущих пустых строк.

Не допускается два и более подряд идущих пробелов, если они не являются частью
строковой константы.

Символ '{' должен всегда быть на новой строчке сразу за оператором к которому он
относится. Размер отступа для него должен соответствовать отступу этого
оператора. Закрывающая скобка '}' должна иметь тот же отступ, что и
соответствующий ей символ '{'.

Если символ '{' соответствует оператору, а не просто начинает новое пространство
имён, то до и после него не должно быть пустых строк. В противном случае пустой
строки не должно быть после него.

Перед '}' не должно быть пустых строк в любом случае.

Например:

    if (a)
	{
	}

Метки case, default, public, private, protected должны иметь тот же отступ, что
и конструкция, к которой они относятcя.

Если метка не находится сразу после строки с '{', то перед ней должна быть
пустая строка.

Например:

    class A
	{
	public:
		A() {}

	private:
		std::list<int> m_b;
	};

Символы '*' и '&' при описании типа данных не должны иметь пробелов слева, но
должны иметь хотя бы один пробел справа.

Например:

    const char* message = "blah-blah-blah";
    const auto& message_ref = message;

### Добавление новых файлов

Если вы добавляете новый файл, всегда вставляйте в его конец специальную строчку
с описанием его формата. Строчка эта - vim mode line. Например, для C/C++
исходных файлов это

    // vim: ts=4 sw=4 tw=0 noet syntax=cpp :

Здесь сообщается информация для текстового редактора Vim, что следует
использовать размер табуляции равный 4 символа, автоотступа - 4 символа, не
использовать автоперенос строк и использовать табуляцию для отступа вместо
пробелов. Плюс указывает какую выбрать подсветку синтаксиса для данного файла.
Даже если вы не используете текстовый редактор Vim, данная строчка служит
напоминанием о формате файла. Вы можете настроить свой текстовый редактор в
соответствии с указанными настройками.

Все новые файлы должны быть так или иначе включены в проект. Для этого нужно
соответствующим образом изменить файл CMakeLists.txt.

### Работа с памятью

Для выделения/освобождения используйте операторы new/delete вместо функций
malloc/calloc/free.

Где это возможно, старайтесь использовать "умные" указатели.

> vim: set ts=4 sw=4 tw=80 et noet :