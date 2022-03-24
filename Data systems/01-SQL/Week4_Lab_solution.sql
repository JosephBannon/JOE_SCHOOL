USE Northwind_DW;



DROP TABLE IF EXISTS FACT_ORDERS;


CREATE TABLE `FACT_ORDERS` (
  `FACT_ORDERS_ID` int NOT NULL AUTO_INCREMENT,
  `ORDER_ID` int DEFAULT NULL,
  `EMPLOYEE_ID` int DEFAULT NULL,
  `CUSTOMER_ID` int DEFAULT NULL,
  `PRODUCT_ID` int DEFAULT NULL,
  `ORDER_DATE` datetime DEFAULT NULL,
  `SHIPPED_DATE` datetime DEFAULT NULL,
  `SHIPPER_ID` int DEFAULT NULL,
  `SHIP_NAME` varchar(50) DEFAULT NULL,
  `SHIP_ADDRESS` longtext,
  `SHIP_CITY` varchar(50) DEFAULT NULL,
  `SHIP_STATE_PROVINCE` varchar(50) DEFAULT NULL,
  `SHIP_ZIP_POSTAL_CODE` varchar(50) DEFAULT NULL,
  `SHIP_COUNTRY_REGION` varchar(50) DEFAULT NULL,
  `SHIPPING_FEE` decimal(19,4) DEFAULT '0.0000',
  `TAXES` decimal(19,4) DEFAULT '0.0000',
  `PAYMENT_TYPE` varchar(50) DEFAULT NULL,
  `PAID_DATE` datetime DEFAULT NULL,
  `TAX_RATE` double DEFAULT '0',
  `QUANTITY` decimal(18,4) DEFAULT '0.0000',
  `UNIT_PRICE` decimal(19,4) DEFAULT '0.0000',
  `DISCOUNT` double DEFAULT '0',
  `ORDER_STATUS` varchar(50) DEFAULT NULL,
  `ORDER_DETAILS_STATUS` varchar(50) DEFAULT NULL,
  PRIMARY KEY (`FACT_ORDERS_ID`),
  KEY `ORDER_ID` (`ORDER_ID`),
  KEY `EMPLOYEE_ID` (`EMPLOYEE_ID`),
  KEY `CUSTOMER_ID` (`CUSTOMER_ID`),
  KEY `PRODUCT_ID` (`PRODUCT_ID`),
  KEY `SHIPPER_ID` (`SHIPPER_ID`)
) ENGINE=InnoDB AUTO_INCREMENT=82 DEFAULT CHARSET=utf8mb4;

USE northwind;

INSERT INTO northwind_dw.FACT_ORDERS
(`ORDER_ID`,`EMPLOYEE_ID`, `CUSTOMER_ID`,`PRODUCT_ID`,`ORDER_DATE`,`SHIPPED_DATE`, `SHIPPER_ID`, `SHIP_NAME`, `SHIP_ADDRESS`, `SHIP_CITY`, `SHIP_STATE_PROVINCE`, 
`SHIP_ZIP_POSTAL_CODE`, `SHIP_COUNTRY_REGION`,`SHIPPING_FEE`, `TAXES`, `PAYMENT_TYPE`, `PAID_DATE`,`TAX_RATE`, `QUANTITY`, `UNIT_PRICE`, `DISCOUNT`, `ORDER_STATUS`, `ORDER_DETAILS_STATUS`
)
SELECT `orders`.`id` AS `ORDER_ID`,
    `orders`.`employee_id` AS `EMPLOYEE_ID`,
    `orders`.`customer_id` AS `CUSTOMER_ID`,
    details.`product_id` AS `PRODUCT_ID`,
    `orders`.`order_date` AS `ORDER_DATE`,
    `orders`.`shipped_date` AS `SHIPPED_DATE`,
    `orders`.`shipper_id` AS `SHIPPER_ID`,
    `orders`.`ship_name` AS `SHIP_NAME`,
    `orders`.`ship_address` AS `SHIP_ADDRESS`,
    `orders`.`ship_city` AS `SHIP_CITY`,
    `orders`.`ship_state_province` AS `SHIP_STATE_PROVINCE`,
    `orders`.`ship_zip_postal_code` AS `SHIP_ZIP_POSTAL_CODE`,
    `orders`.`ship_country_region` AS `SHIP_COUNTRY_REGION`,
    `orders`.`shipping_fee` AS `SHIPPING_FEE`,
    `orders`.`taxes` AS `TAXES`,
    `orders`.`payment_type` AS `PAYMENT_TYPE`,
    `orders`.`paid_date` AS `PAID_DATE`,
    `orders`.`tax_rate` `TAX_RATE`,
    details.quantity as `QUANTITY`,
    details.unit_price as `UNIT_PRICE`,
details.discount as `DISCOUNT`, 
`status`.status_name  as `ORDER_STATUS`, 
details_status.status_name as `ORDER_DETAILS_STATUS`
FROM northwind.orders orders
RIGHT OUTER JOIN northwind.details ON orders.id = details.order_id
INNER JOIN northwind.orders_status `status` ON orders.status_id = `status`.id
INNER JOIN northwind.order_details_status details_status ON details.status_id = details_status.id;


SELECT * FROM northwind_dw.fact_orders
ORDER BY FACT_ORDERS_ID;
