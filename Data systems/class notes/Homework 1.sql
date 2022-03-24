#1)
SELECT product_name, quantity_per_unit
FROM products;

#2)
SELECT id as product_id, product_name
FROM products;

#3)
SELECT id as product_id, product_name
FROM products
WHERE discontinued = 1;

#4)
SELECT product_name, list_price
FROM products
WHERE list_price IN (SELECT MIN(list_price) FROM products)
OR list_price IN (SELECT MAX(list_price) FROM products);

#5)
SELECT id as product_id, product_name, list_price
FROM products
WHERE list_price < 20;

#6)
SELECT id as product_id, product_name, list_price
FROM products
WHERE list_price BETWEEN 15 AND 25;

#7)
SELECT product_name, list_price
FROM products
WHERE list_price > (SELECT AVG(list_price) FROM products);

#8)
SELECT product_name, list_price
FROM products
ORDER BY list_price DESC 
LIMIT 10;

#9)
SELECT discontinued, COUNT(*) AS 'Number of products'
FROM products
GROUP BY discontinued;

#9)
SELECT REPLACE(discontinued,0,"current") as 'product type' , COUNT(*) AS 'Number of products'
FROM products
GROUP BY discontinued;


#10)
SELECT product_name, reorder_level AS units_on_order, target_level AS units_in_stock
FROM products
WHERE reorder_level < target_level;

