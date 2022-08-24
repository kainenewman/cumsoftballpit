
## Javascript ##


//https://zero.p2p.shpp.me/tasks/4_1_cash_machine.html
let bankCards = [];
let continueQuestion = true;
// let cardNumber; // переніс поближче до місця застосування
// let moneyToPutOnCard; // переніс поближче до місця застосування
let sum = 0;
for (let i = 0; i < 10; i++) { //Присваиваем каждому элементу массива значение 0
    bankCards[i] = 0;
}
while (continueQuestion == true) {
    let cardNumber; 
	do {
        cardNumber = +prompt("Введите номер карты от 0 до 9");
    } while (cardNumber < 0 || cardNumber > 9) // зручніше уявляти числа у натуральному порядкові 
   	
    let moneyToPutOnCard;
    do {
        moneyToPutOnCard = +prompt("Сколько положить?");
    } while ( moneyToPutOnCard < -1000 || moneyToPutOnCard > 1000)
    bankCards[cardNumber] += moneyToPutOnCard; //Присваиваем элементу массива (введенному номеру карты) указанное количество денег Тут плючік здається зайвий.
    sum += moneyToPutOnCard //Находим сумму на всех картах - ну це не сума на всіх картках, це введений платіж
    continueQuestion = confirm("Продолжить пополнение?");
}
let counter = 0; // та можна просто і
while (counter < 10) {
    console.put(bankCards[counter] + " "); //Выводим количество денег на каждой карте
    counter++;
}
console.log("\nВ сумме на всех картах " + sum); //Выводим сумму на всех картах

