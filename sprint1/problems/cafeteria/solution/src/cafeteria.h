#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

using namespace std::literals;

namespace net = boost::asio;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;


class Order : public std::enable_shared_from_this<Order> {
public:
    Order(net::io_context& io, std::shared_ptr<Sausage> sausage, std::shared_ptr<Bread> bread,
          std::shared_ptr<GasCooker> gas_cooker, HotDogHandler handler)         : io_(io),
                                                                                  sausage_(sausage),
                                                                                  bread_(bread),
                                                                                  gas_cooker_(std::move(gas_cooker)),
                                                                                  handler_(std::move(handler)) {}


    void Execute() {
        sausage_->StartFry(*gas_cooker_, [order = shared_from_this()] {
            //Когда началась готовка, заводим таймер
            //Когда таймер сработал, говорим заказу, что что-то готово
            //Когда готовы оба ингридиента, делаем хот-дог
            //Чтоб не читать и менять is_some_ready одновременно
            //Делаем это последовательно в strand_

            //Сейчас сосиска начала жариться...
            //Заводим таймер на 1.5 секунд
            auto timer = std::make_shared<net::steady_timer>(order->GetContext(), 1500ms);

            timer->async_wait([timer, order](sys::error_code ec){
                //Сейчас сосиска дожарилась
                //Сообщаем об этом order
                //Если булочка готова к этому моменту, order вызовет handler
                order->GetSausage().StopFry();
                net::post(order->GetStrand(), [order]{
                    order->Ready();
                });
            });
        });

        bread_->StartBake(*gas_cooker_, [order = shared_from_this()] {
            auto timer = std::make_shared<net::steady_timer>(order->GetContext(), 1s);

            timer->async_wait([timer, order](sys::error_code ec){
                order->GetBread().StopBaking();
                net::post(order->GetStrand(), [order]{
                    order->Ready();
                });
            });
        });

    }

    net::strand<net::io_context::executor_type>& GetStrand() {
        return strand_;
    }

    net::io_context& GetContext() {
        return io_;
    }

    Sausage& GetSausage() {
        return *sausage_;
    }

    Bread& GetBread() {
        return *bread_;
    }

    void Ready() {
        if (is_some_ready_) {
            HotDog result(sausage_->GetId(), sausage_, bread_);
            handler_(Result(std::move(result)));
        } else {
            is_some_ready_ = true;
        }
    }

private:
    net::io_context& io_;
    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<Bread> bread_;
    std::shared_ptr<GasCooker> gas_cooker_;
    HotDogHandler handler_;
    bool is_some_ready_ = false;
    net::strand<net::io_context::executor_type> strand_ = net::make_strand(io_);
};


// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {



        net::dispatch(ingr_strand, [&store = store_, handler = std::move(handler), &io = io_, &gas_cooker = gas_cooker_]{
            auto sausage = store.GetSausage();
            auto bread   = store.GetBread();

            auto ord = std::make_shared<Order>(io, std::move(sausage), std::move(bread),
                                               gas_cooker, std::move(handler));

            ord->Execute();
        });

        // TODO: Реализуйте метод самостоятельно
        // При необходимости реализуйте дополнительные классы
    }

private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
    net::strand<net::io_context::executor_type> ingr_strand = net::make_strand(io_);
};
